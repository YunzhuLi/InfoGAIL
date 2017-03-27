from utils import *
import numpy as np
import time
import math
import argparse
from keras.initializations import normal, identity, uniform
from keras.models import model_from_json
from keras.models import Sequential, Model
from keras.layers import Dense, BatchNormalization, Activation, Convolution2D, MaxPooling2D, Flatten, Input, merge, Lambda
from keras.layers.advanced_activations import LeakyReLU
from keras.optimizers import Adam, RMSprop
import tensorflow as tf
import keras.backend as K
import json

from keras.applications.resnet50 import ResNet50
from keras.preprocessing import image
from keras.applications.resnet50 import preprocess_input
from keras.utils.np_utils import to_categorical


parser = argparse.ArgumentParser(description="TRPO")
parser.add_argument("--paths_per_collect", type=int, default=10)
parser.add_argument("--max_step_limit", type=int, default=200)
parser.add_argument("--pre_step", type=int, default=120)
parser.add_argument("--n_iter", type=int, default=1000)
parser.add_argument("--gamma", type=float, default=.95)
parser.add_argument("--lam", type=float, default=.97)
parser.add_argument("--max_kl", type=float, default=0.01)
parser.add_argument("--cg_damping", type=float, default=0.1)
parser.add_argument("--lr_discriminator", type=float, default=5e-5)
parser.add_argument("--d_iter", type=int, default=100)
parser.add_argument("--clamp_lower", type=float, default=-0.01)
parser.add_argument("--clamp_upper", type=float, default=0.01)
parser.add_argument("--lr_baseline", type=float, default=1e-4)
parser.add_argument("--b_iter", type=int, default=25)
parser.add_argument("--lr_posterior", type=float, default=1e-4)
parser.add_argument("--p_iter", type=int, default=50)
parser.add_argument("--buffer_size", type=int, default=75)
parser.add_argument("--sample_size", type=int, default=50)
parser.add_argument("--batch_size", type=int, default=500)

args = parser.parse_args()


class TRPOAgent(object):
    config = dict2(paths_per_collect = args.paths_per_collect,
                   max_step_limit = args.max_step_limit,
                   pre_step = args.pre_step,
                   n_iter = args.n_iter,
                   gamma = args.gamma,
                   lam = args.lam,
                   max_kl = args.max_kl,
                   cg_damping = args.cg_damping,
                   lr_discriminator = args.lr_discriminator,
                   d_iter = args.d_iter,
                   clamp_lower = args.clamp_lower,
                   clamp_upper = args.clamp_upper,
                   lr_baseline = args.lr_baseline,
                   b_iter = args.b_iter,
                   lr_posterior = args.lr_posterior,
                   p_iter = args.p_iter,
                   buffer_size = args.buffer_size,
                   sample_size = args.sample_size,
                   batch_size = args.batch_size)

    def __init__(self, env, sess, feat_dim, aux_dim, encode_dim, action_dim,
                 img_dim, pre_actions):
        self.env = env
        self.sess = sess
        self.buffer = ReplayBuffer(self.config.buffer_size)
        self.feat_dim = feat_dim
        self.aux_dim = aux_dim
        self.encode_dim = encode_dim
        self.action_dim = action_dim
        self.img_dim = img_dim
        self.pre_actions = pre_actions
        self.feats = feats = tf.placeholder(
            dtype, shape=[None, feat_dim[0], feat_dim[1], feat_dim[2]]
        )
        self.auxs = auxs = tf.placeholder(dtype, shape=[None, aux_dim])
        self.encodes = encodes = tf.placeholder(dtype, shape=[None, encode_dim])
        self.actions = actions = tf.placeholder(dtype, shape=[None, action_dim])

        self.advants = advants = tf.placeholder(dtype, shape=[None])
        self.oldaction_dist_mu = oldaction_dist_mu = \
                tf.placeholder(dtype, shape=[None, action_dim])
        self.oldaction_dist_logstd = oldaction_dist_logstd = \
                tf.placeholder(dtype, shape=[None, action_dim])

        # Create neural network.
        print "Now we build trpo generator"
        self.generator = self.create_generator(feats, auxs, encodes)
        print "Now we build discriminator"
        self.discriminator, self.discriminate = \
            self.create_discriminator(img_dim, aux_dim, action_dim)
        print "Now we build posterior"
        self.posterior = \
            self.create_posterior(img_dim, aux_dim, action_dim, encode_dim)
        self.posterior_target = \
            self.create_posterior(img_dim, aux_dim, action_dim, encode_dim)

        self.demo_idx = 0

        action_dist_mu = self.generator.outputs[0]
        # self.action_dist_logstd_param = action_dist_logstd_param = \
        #         tf.placeholder(dtype, shape=[1, action_dim])
        # action_dist_logstd = tf.tile(action_dist_logstd_param,
        #                              tf.pack((tf.shape(action_dist_mu)[0], 1)))
        action_dist_logstd = tf.placeholder(dtype, shape=[None, action_dim])

        eps = 1e-8
        self.action_dist_mu = action_dist_mu
        self.action_dist_logstd = action_dist_logstd
        N = tf.shape(feats)[0]
        # compute probabilities of current actions and old actions
        log_p_n = gauss_log_prob(action_dist_mu, action_dist_logstd, actions)
        log_oldp_n = gauss_log_prob(oldaction_dist_mu, oldaction_dist_logstd, actions)

        ratio_n = tf.exp(log_p_n - log_oldp_n)
        Nf = tf.cast(N, dtype)
        surr = -tf.reduce_mean(ratio_n * advants) # Surrogate loss
        var_list = self.generator.trainable_weights

        kl = gauss_KL(oldaction_dist_mu, oldaction_dist_logstd,
                      action_dist_mu, action_dist_logstd) / Nf
        ent = gauss_ent(action_dist_mu, action_dist_logstd) / Nf

        self.losses = [surr, kl, ent]
        self.pg = flatgrad(surr, var_list)
        # KL divergence where first arg is fixed
        kl_firstfixed = gauss_selfKL_firstfixed(action_dist_mu,
                                                action_dist_logstd) / Nf
        grads = tf.gradients(kl_firstfixed, var_list)
        self.flat_tangent = tf.placeholder(dtype, shape=[None])
        shapes = map(var_shape, var_list)
        start = 0
        tangents = []
        for shape in shapes:
            size = np.prod(shape)
            param = tf.reshape(self.flat_tangent[start:(start + size)], shape)
            tangents.append(param)
            start += size
        gvp = [tf.reduce_sum(g * t) for (g, t) in zip(grads, tangents)]
        self.fvp = flatgrad(gvp, var_list)
        self.gf = GetFlat(self.sess, var_list)
        self.sff = SetFromFlat(self.sess, var_list)
        self.baseline = NNBaseline(sess, feat_dim, aux_dim, encode_dim,
                                   self.config.lr_baseline, self.config.b_iter,
                                   self.config.batch_size)
        self.sess.run(tf.global_variables_initializer())

        # Create feature extractor
        self.base_model = ResNet50(weights='imagenet', include_top=False)
        self.feat_extractor = Model(
            input=self.base_model.input,
            output=self.base_model.get_layer('activation_40').output
        )

    def create_generator(self, feats, auxs, encodes):
        feats = Input(tensor=feats)
        x = Convolution2D(256, 3, 3)(feats)
        x = LeakyReLU()(x)
        x = Convolution2D(256, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Flatten()(x)
        auxs = Input(tensor=auxs)
        h = merge([x, auxs], mode='concat')
        h = Dense(256)(h)
        h = LeakyReLU()(h)
        h = Dense(128)(h)
        encodes = Input(tensor=encodes)
        c = Dense(128)(encodes)
        h = merge([h, c], mode='sum')
        h = LeakyReLU()(h)

        steer = Dense(1, activation='tanh', init=lambda shape, name:
                      normal(shape, scale=1e-4, name=name))(h)
        accel = Dense(1, activation='sigmoid', init=lambda shape, name:
                             normal(shape, scale=1e-4, name=name))(h)
        brake = Dense(1, activation='sigmoid', init=lambda shape, name:
                      normal(shape, scale=1e-4, name=name))(h)
        actions = merge([steer, accel, brake], mode='concat')
        model = Model(input=[feats, auxs, encodes], output=actions)
        return model

    def create_discriminator(self, img_dim, aux_dim, action_dim):
        imgs = Input(shape=[img_dim[0], img_dim[1], img_dim[2]])
        x = Convolution2D(32, 3, 3, subsample=(2, 2))(imgs)
        x = LeakyReLU()(x)
        x = Convolution2D(64, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Convolution2D(128, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Flatten()(x)
        auxs = Input(shape=[aux_dim])
        actions = Input(shape=[action_dim])
        h = merge([x, auxs, actions], mode='concat')
        h = Dense(256)(h)
        h = LeakyReLU()(h)
        h = Dense(128)(h)
        h = LeakyReLU()(h)
        p = Dense(1)(h)
        discriminate = Model(input=[imgs, auxs, actions], output=p)

        imgs_n = Input(shape=[img_dim[0], img_dim[1], img_dim[2]])
        imgs_d = Input(shape=[img_dim[0], img_dim[1], img_dim[2]])
        auxs_n = Input(shape=[aux_dim])
        auxs_d = Input(shape=[aux_dim])
        actions_n = Input(shape=[action_dim])
        actions_d = Input(shape=[action_dim])

        p_n = discriminate([imgs_n, auxs_n, actions_n])
        p_d = discriminate([imgs_d, auxs_d, actions_d])
        p_d = Lambda(lambda x: -x)(p_d)
        p_output = merge([p_n, p_d], mode='sum')

        model = Model(input=[imgs_n, auxs_n, actions_n,
                             imgs_d, auxs_d, actions_d],
                      output=p_output)
        rmsprop = RMSprop(lr=self.config.lr_discriminator)
        model.compile(
            # little trick to use Keras predefined lambda loss function
            loss=lambda y_pred, p_true: K.mean(y_pred * p_true), optimizer=rmsprop
        )

        return model, discriminate

    def create_posterior(self, img_dim, aux_dim, action_dim, encode_dim):
        imgs = Input(shape=[img_dim[0], img_dim[1], img_dim[2]])
        x = Convolution2D(32, 3, 3, subsample=(2, 2))(imgs)
        x = LeakyReLU()(x)
        x = Convolution2D(64, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Convolution2D(128, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Flatten()(x)
        auxs = Input(shape=[aux_dim])
        actions = Input(shape=[action_dim])
        h = merge([x, auxs, actions], mode='concat')
        h = Dense(256)(h)
        h = LeakyReLU()(h)
        h = Dense(128)(h)
        h = LeakyReLU()(h)
        c = Dense(encode_dim, activation='softmax')(h)

        model = Model(input=[imgs, auxs, actions], output=c)
        adam = Adam(lr=self.config.lr_posterior)
        model.compile(loss='categorical_crossentropy', optimizer=adam,
                      metrics=['accuracy'])
        return model

    def act(self, feats, auxs, encodes, logstds, *args):
        action_dist_mu = \
                self.sess.run(
                    self.action_dist_mu,
                    {self.feats: feats, self.auxs: auxs, self.encodes: encodes}
                )

        act = action_dist_mu + np.exp(logstds) * \
                np.random.randn(*logstds.shape)

        act[:, 0] = np.clip(act[:, 0], -1, 1)
        act[:, 1] = np.clip(act[:, 1], 0, 1)
        act[:, 2] = np.clip(act[:, 2], 0, 1)

        return act

    def learn(self, demo):
        config = self.config
        start_time = time.time()
        numeptotal = 0

        # Set up for training discrimiator
        print "Loading data ..."
        imgs_d, auxs_d, actions_d = demo["imgs"], demo["auxs"], demo["actions"]
        numdetotal = imgs_d.shape[0]
        idx_d = np.arange(numdetotal)
        np.random.shuffle(idx_d)

        imgs_d = imgs_d[idx_d]
        auxs_d = auxs_d[idx_d]
        actions_d = actions_d[idx_d]
        print "Resizing img for demo ..."
        imgs_reshaped_d = []
        for i in xrange(numdetotal):
            imgs_reshaped_d.append(np.expand_dims(cv2.resize(imgs_d[i],
                (self.img_dim[0], self.img_dim[1])), axis=0))
        imgs_d = np.concatenate(imgs_reshaped_d, axis=0).astype(np.float32)
        imgs_d = (imgs_d - 128.) / 128.
        print "Shape of resized demo images:", imgs_d.shape

        for i in xrange(1, config.n_iter):

            # Generating paths.
            if i == 1:
                paths_per_collect = 30
            else:
                paths_per_collect = 10
            rollouts = rollout_contin(
                self.env,
                self,
                self.feat_extractor,
                self.feat_dim,
                self.aux_dim,
                self.encode_dim,
                config.max_step_limit,
                config.pre_step,
                paths_per_collect,
                self.pre_actions,
                self.discriminate,
                self.posterior_target)

            for path in rollouts:
                self.buffer.add(path)
            print "Buffer count:", self.buffer.count()

            paths = self.buffer.get_sample(config.sample_size)

            print "Calculating actions ..."
            for path in paths:
                path["mus"] = self.sess.run(
                    self.action_dist_mu,
                    {self.feats: path["feats"],
                     self.auxs: path["auxs"],
                     self.encodes: path["encodes"]}
                )

            mus_n = np.concatenate([path["mus"] for path in paths])
            logstds_n = np.concatenate([path["logstds"] for path in paths])
            feats_n = np.concatenate([path["feats"] for path in paths])
            auxs_n = np.concatenate([path["auxs"] for path in paths])
            encodes_n = np.concatenate([path["encodes"] for path in paths])
            actions_n = np.concatenate([path["actions"] for path in paths])
            imgs_n = np.concatenate([path["imgs"] for path in paths])

            print "Epoch:", i, "Total sampled data points:", feats_n.shape[0]

            # Train discriminator
            numnototal = feats_n.shape[0]
            batch_size = config.batch_size
            start_d = self.demo_idx
            start_n = 0
            if i <= 5:
                d_iter = 120 - i * 20
            else:
                d_iter = 20
            for k in xrange(d_iter):
                loss = self.discriminator.train_on_batch(
                    [imgs_n[start_n:start_n + batch_size],
                     auxs_n[start_n:start_n + batch_size],
                     actions_n[start_n:start_n + batch_size],
                     imgs_d[start_d:start_d + batch_size],
                     auxs_d[start_d:start_d + batch_size],
                     actions_d[start_d:start_d + batch_size]],
                    np.ones(batch_size)
                )
                # print self.discriminator.summary()
                for l in self.discriminator.layers:
                    weights = l.get_weights()
                    weights = [np.clip(w, config.clamp_lower, config.clamp_upper)
                               for w in weights]
                    l.set_weights(weights)

                start_d = self.demo_idx = self.demo_idx + batch_size
                start_n = start_n + batch_size

                if start_d + batch_size >= numdetotal:
                    start_d = self.demo_idx = (start_d + batch_size) % numdetotal
                if start_n + batch_size >= numnototal:
                    start_n = (start_n + batch_size) % numnototal

                print "Discriminator step:", k, "loss:", loss

            idx = np.arange(numnototal)
            np.random.shuffle(idx)
            train_val_ratio = 0.7
            # Training data for posterior
            numno_train = int(numnototal * train_val_ratio)
            imgs_train = imgs_n[idx][:numno_train]
            auxs_train = auxs_n[idx][:numno_train]
            actions_train = actions_n[idx][:numno_train]
            encodes_train = encodes_n[idx][:numno_train]
            # Validation data for posterior
            imgs_val = imgs_n[idx][numno_train:]
            auxs_val = auxs_n[idx][numno_train:]
            actions_val = actions_n[idx][numno_train:]
            encodes_val = encodes_n[idx][numno_train:]

            start_n = 0
            for j in xrange(config.p_iter):
                loss = self.posterior.train_on_batch(
                    [imgs_train[start_n:start_n + batch_size],
                     auxs_train[start_n:start_n + batch_size],
                     actions_train[start_n:start_n + batch_size]],
                    encodes_train[start_n:start_n + batch_size]
                )
                start_n += batch_size
                if start_n + batch_size >= numno_train:
                    start_n = (start_n + batch_size) % numno_train

                posterior_weights = self.posterior.get_weights()
                posterior_target_weights = self.posterior_target.get_weights()
                for k in xrange(len(posterior_weights)):
                    posterior_target_weights[k] = 0.5 * posterior_weights[k] +\
                            0.5 * posterior_target_weights[k]
                self.posterior_target.set_weights(posterior_target_weights)

                output_p = self.posterior_target.predict(
                    [imgs_val, auxs_val, actions_val])
                val_loss = -np.average(
                    np.sum(np.log(output_p) * encodes_val, axis=1))
                print "Posterior step:", j, "loss:", loss, val_loss

            # Computing returns and estimating advantage function.
            path_idx = 0
            for path in paths:
                file_path = "/home/yunzhu/Desktop/log/iter_%d_path_%d.txt" % (i, path_idx)
                f = open(file_path, "w")
                path["baselines"] = self.baseline.predict(path)
                output_d = self.discriminate.predict(
                    [path["imgs"], path["auxs"], path["actions"]])
                output_p = self.posterior_target.predict(
                    [path["imgs"], path["auxs"], path["actions"]])
                path["rewards"] = np.ones(path["raws"].shape[0]) * 1.2 + \
                        output_d.flatten() * 0.2 + \
                        np.sum(np.log(output_p) * path["encodes"], axis=1)

                path_baselines = np.append(path["baselines"], 0 if
                                           path["baselines"].shape[0] == 100 else
                                           path["baselines"][-1])
                deltas = path["rewards"] + config.gamma * path_baselines[1:] -\
                        path_baselines[:-1]
                # path["returns"] = discount(path["rewards"], config.gamma)
                # path["advants"] = path["returns"] - path["baselines"]
                path["advants"] = discount(deltas, config.gamma * config.lam)
                path["returns"] = discount(path["rewards"], config.gamma)

                f.write("Baseline:\n" + np.array_str(path_baselines) + "\n")
                f.write("Returns:\n" + np.array_str(path["returns"]) + "\n")
                f.write("Advants:\n" + np.array_str(path["advants"]) + "\n")
                f.write("Mus:\n" + np.array_str(path["mus"]) + "\n")
                f.write("Actions:\n" + np.array_str(path["actions"]) + "\n")
                f.write("Logstds:\n" + np.array_str(path["logstds"]) + "\n")
                path_idx += 1

            # Standardize the advantage function to have mean=0 and std=1
            advants_n = np.concatenate([path["advants"] for path in paths])
            # advants_n -= advants_n.mean()
            advants_n /= (advants_n.std() + 1e-8)

            # Computing baseline function for next iter.
            self.baseline.fit(paths)

            feed = {self.feats: feats_n,
                    self.auxs: auxs_n,
                    self.encodes: encodes_n,
                    self.actions: actions_n,
                    self.advants: advants_n,
                    self.action_dist_logstd: logstds_n,
                    self.oldaction_dist_mu: mus_n,
                    self.oldaction_dist_logstd: logstds_n}

            thprev = self.gf()

            def fisher_vector_product(p):
                feed[self.flat_tangent] = p
                return self.sess.run(self.fvp, feed) + p * config.cg_damping

            g = self.sess.run(self.pg, feed_dict=feed)
            stepdir = conjugate_gradient(fisher_vector_product, -g)
            shs = .5 * stepdir.dot(fisher_vector_product(stepdir))
            assert shs > 0

            lm = np.sqrt(shs / config.max_kl)
            fullstep = stepdir / lm
            neggdotstepdir = -g.dot(stepdir)

            def loss(th):
                self.sff(th)
                return self.sess.run(self.losses[0], feed_dict=feed)
            theta = linesearch(loss, thprev, fullstep, neggdotstepdir / lm)
            self.sff(theta)

            surrafter, kloldnew, entropy = self.sess.run(
                self.losses, feed_dict=feed
            )

            episoderewards = np.array([path["rewards"].sum() for path in paths])
            stats = {}
            numeptotal += len(episoderewards)
            stats["Total number of episodes"] = numeptotal
            stats["Average sum of rewards per episode"] = episoderewards.mean()
            stats["Entropy"] = entropy
            stats["Time elapsed"] = "%.2f mins" % ((time.time() - start_time) / 60.0)
            stats["KL between old and new distribution"] = kloldnew
            stats["Surrogate loss"] = surrafter
            print("\n********** Iteration {} **********".format(i))
            for k, v in stats.iteritems():
                print(k + ": " + " " * (40 - len(k)) + str(v))
            if entropy != entropy:
                exit(-1)

            param_dir = "/home/yunzhu/Desktop/params/"
            print("Now we save model")
            self.generator.save_weights(
                param_dir + "generator_model_%d.h5" % i, overwrite=True)
            with open(param_dir + "generator_model_%d.json" % i, "w") as outfile:
                json.dump(self.generator.to_json(), outfile)

            self.discriminator.save_weights(
                param_dir + "discriminator_model_%d.h5" % i, overwrite=True)
            with open(param_dir + "discriminator_model_%d.json" % i, "w") as outfile:
                json.dump(self.discriminator.to_json(), outfile)

            self.baseline.model.save_weights(
                param_dir + "baseline_model_%d.h5" % i, overwrite=True)
            with open(param_dir + "baseline_model_%d.json" % i, "w") as outfile:
                json.dump(self.baseline.model.to_json(), outfile)

            self.posterior.save_weights(
                param_dir + "posterior_model_%d.h5" % i, overwrite=True)
            with open(param_dir + "posterior_model_%d.json" % i, "w") as outfile:
                json.dump(self.posterior.to_json(), outfile)

            self.posterior_target.save_weights(
                param_dir + "posterior_target_model_%d.h5" % i, overwrite=True)
            with open(param_dir + "posterior_target_model_%d.json" % i, "w") as outfile:
                json.dump(self.posterior_target.to_json(), outfile)



class Generator(object):
    def __init__(self, sess, feat_dim, aux_dim, encode_dim, action_dim):
        self.sess = sess
        self.lr = tf.placeholder(tf.float32, shape=[])

        K.set_session(sess)

        self.model, self.weights, self.feats, self.auxs, self.encodes = \
                self.create_generator(feat_dim, aux_dim, encode_dim)

        self.action_gradient = tf.placeholder(tf.float32, [None, action_dim])
        self.params_grad = tf.gradients(self.model.output, self.weights,
                                        self.action_gradient)
        grads = zip(self.params_grad, self.weights)
        self.optimize = tf.train.AdamOptimizer(self.lr).apply_gradients(grads)

        self.sess.run(tf.global_variables_initializer())

    def train(self, feats, auxs, encodes, action_grads, lr):
        self.sess.run(self.optimize, feed_dict={
            self.feats: feats,
            self.auxs: auxs,
            self.encodes: encodes,
            self.lr: lr,
            self.action_gradient: action_grads,
            K.learning_phase(): 1
        })

    def create_generator(self, feat_dim, aux_dim, encode_dim):
        feats = Input(shape=[feat_dim[0], feat_dim[1], feat_dim[2]])
        x = Convolution2D(256, 3, 3)(feats)
        x = LeakyReLU()(x)
        x = Convolution2D(256, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Flatten()(x)
        auxs = Input(shape=[aux_dim])
        h = merge([x, auxs], mode='concat')
        h = Dense(256)(h)
        h = LeakyReLU()(h)
        h = Dense(128)(h)
        encodes = Input(shape=[encode_dim])
        c = Dense(128)(encodes)
        h = merge([h, c], mode='sum')
        h = LeakyReLU()(h)

        steer = Dense(1, activation='tanh', init=lambda shape, name:
                      normal(shape, scale=1e-4, name=name))(h)
        accel = Dense(1, activation='sigmoid', init=lambda shape, name:
                             normal(shape, scale=1e-4, name=name))(h)
        brake = Dense(1, activation='sigmoid', init=lambda shape, name:
                      normal(shape, scale=1e-4, name=name))(h)
        actions = merge([steer, accel, brake], mode='concat')
        model = Model(input=[feats, auxs, encodes], output=actions)
        return model, model.trainable_weights, feats, auxs, encodes


class Posterior(object):
    def __init__(self, sess, img_dim, aux_dim, action_dim, encode_dim):
        self.sess = sess
        self.lr = tf.placeholder(tf.float32, shape=[])

        K.set_session(sess)

        self.model = self.create_posterior(img_dim, aux_dim, action_dim, encode_dim)

    def create_posterior(self, img_dim, aux_dim, action_dim, encode_dim):
        imgs = Input(shape=[img_dim[0], img_dim[1], img_dim[2]])
        x = Convolution2D(32, 3, 3, subsample=(2, 2))(imgs)
        x = LeakyReLU()(x)
        x = Convolution2D(64, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Convolution2D(128, 3, 3, subsample=(2, 2))(x)
        x = LeakyReLU()(x)
        x = Flatten()(x)
        auxs = Input(shape=[aux_dim])
        actions = Input(shape=[action_dim])
        h = merge([x, auxs, actions], mode='concat')
        h = Dense(256)(h)
        h = LeakyReLU()(h)
        h = Dense(128)(h)
        h = LeakyReLU()(h)
        c = Dense(encode_dim, activation='softmax')(h)

        model = Model(input=[imgs, auxs, actions], output=c)
        return model
