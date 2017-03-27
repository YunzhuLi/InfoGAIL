import numpy as np
import tensorflow as tf
import json
import time
from keras.applications.resnet50 import ResNet50, preprocess_input
from keras.models import Model
from models import Generator


def calc_loss(act_gt, act_pred):
    steer_loss = np.average(np.abs(act_gt[:, 0] - act_pred[:, 0]))
    accel_loss = np.average(np.abs(act_gt[:, 1] - act_pred[:, 1]))
    brake_loss = np.average(np.abs(act_gt[:, 2] - act_pred[:, 2]))
    return np.array([steer_loss, accel_loss, brake_loss])


def get_feat(imgs, feat_extractor):
    x = preprocess_input(imgs.astype(np.float32))
    x = feat_extractor.predict(x)
    return x


def main():
    param_dir = "/home/yunzhu/Desktop/wgail_info_low_1_params/params/"
    demo_dir = "/home/yunzhu/Desktop/human_low_case_1/"
    feat_dim = [7, 13, 1024]
    aux_dim = 10
    encode_dim = 2
    action_dim = 3
    batch_size = 640
    train_val_ratio = 0.8
    lr = 0.0001
    lr_decay_factor = .99

    episode = 5000

    np.random.seed(1024)

    config = tf.ConfigProto()
    config.gpu_options.allow_growth = True
    sess = tf.Session(config=config)
    from keras import backend as K
    K.set_session(sess)

    print "Now we build generator"
    generator = Generator(sess, feat_dim, aux_dim, encode_dim, action_dim)
    print generator.model.summary()
    print "Now we build feature extractor"
    base_model = ResNet50(weights='imagenet', include_top=False)
    feat_extractor = Model(
        input=base_model.input,
        output=base_model.get_layer('activation_40').output
    )

    print "Loading data ..."
    raw = np.load(demo_dir + "demo.npz")
    imgs, auxs, actions = raw["imgs"], raw["auxs"], raw["actions"]
    num_data = imgs.shape[0]
    idx = np.arange(num_data)
    np.random.shuffle(idx)

    imgs_train = imgs[idx][:int(num_data * train_val_ratio)]
    auxs_train = auxs[idx][:int(num_data * train_val_ratio)]
    actions_train = actions[idx][:int(num_data * train_val_ratio)]
    imgs_val = imgs[idx][int(num_data * train_val_ratio):]
    auxs_val = auxs[idx][int(num_data * train_val_ratio):]
    actions_val = actions[idx][int(num_data * train_val_ratio):]

    print "Getting feature for training set ..."
    feats_train = get_feat(imgs_train, feat_extractor)
    print "Getting feature for validation set ..."
    feats_val = get_feat(imgs_val, feat_extractor)

    cur_min_loss_val = 1.

    for i in xrange(episode):
        total_step = imgs_train.shape[0] // batch_size

        train_loss = np.array([0., 0., 0.])
        for j in xrange(total_step):
            feats_cur = feats_train[j * batch_size : (j + 1) * batch_size]
            auxs_cur = auxs_train[j * batch_size : (j + 1) * batch_size]
            encodes_cur = np.zeros([batch_size, encode_dim], dtype=np.float32)
            idx = np.random.randint(0, encode_dim, batch_size)
            encodes_cur[np.arange(batch_size), idx] = 1

            act_pred = generator.model.predict([feats_cur, auxs_cur,
                                                encodes_cur])
            act_gt = actions_train[j * batch_size : (j + 1) * batch_size]

            generator.train(feats_cur, auxs_cur, encodes_cur,
                            act_pred - act_gt, lr)
            batch_loss = calc_loss(act_gt, act_pred)
            print "Episode:", i, "Batch:", j, "/", total_step, \
                    np.round(batch_loss, 6), np.sum(batch_loss)

            train_loss += batch_loss / total_step

        if i % 20 == 0 and i > 0:
            lr *= lr_decay_factor

        encodes_val = np.zeros([feats_val.shape[0], encode_dim], dtype=np.float32)
        idx = np.random.randint(0, encode_dim, feats_val.shape[0])
        encodes_val[idx] = 1

        act_pred = generator.model.predict([feats_val, auxs_val, encodes_val])
        val_loss = calc_loss(actions_val, act_pred)
        print "Episode:", i, \
            "Train Loss: ", np.round(train_loss, 6), np.sum(train_loss), \
            "Test Loss:", np.round(val_loss, 6), np.sum(val_loss), cur_min_loss_val, \
            "LR:", lr

        if cur_min_loss_val > np.sum(val_loss):
            cur_min_loss_val = np.sum(val_loss)
            print("Now we save the model")
            generator.model.save_weights(param_dir + "generator_bc_model.h5", overwrite=True)
            with open(param_dir + "generator_bc_model.json", "w") as outfile:
                json.dump(generator.model.to_json(), outfile)


if __name__ == "__main__":
    main()
