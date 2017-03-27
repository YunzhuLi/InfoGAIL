from keras.applications.resnet50 import ResNet50
from keras.preprocessing import image
from keras.applications.resnet50 import preprocess_input
from PIL import Image
import numpy as np
import time
import tensorflow as tf
from gym_torcs import TorcsEnv
import json
from models import Generator
from keras.models import Model
import cv2

code = 1

feat_dim = [7, 13, 1024]
aux_dim = 10
encode_dim = 2
action_dim = 3
pre_actions_path = "/home/yunzhu/Desktop/human_1/pre_actions.npz"
param_path = "/home/yunzhu/Desktop/wgail_info_params_1/params/generator_model_30.h5"

MAX_STEP_LIMIT = 200
PRE_STEP = 120


def clip(v, lo, hi):
    if v < lo: return lo
    elif v > hi: return hi
    else: return v


def get_state(ob, aux_dim, feat_extractor):
    img = ob.img
    img[:, :, [0, 2]] = img[:, :, [2, 0]]
    img = cv2.resize(img, (200, 150))
    img = img[40:, :, :]
    x = np.expand_dims(img, axis=0).astype(np.float32)
    x = preprocess_input(x)
    feat = feat_extractor.predict(x)

    aux = np.zeros(aux_dim, dtype=np.float32)
    aux[0] = ob.damage
    aux[1] = ob.speedX
    aux[2] = ob.speedY
    aux[3] = ob.speedZ
    aux[4:7] = ob.pre_action_0
    aux[7:10] = ob.pre_action_1
    return feat, np.expand_dims(aux, axis=0)


def main():
    config = tf.ConfigProto()
    config.gpu_options.allow_growth = True
    sess = tf.Session(config=config)
    from keras import backend as K
    K.set_session(sess)

    generator = Generator(sess, feat_dim, aux_dim, encode_dim, action_dim)
    base_model = ResNet50(weights='imagenet', include_top=False)
    feat_extractor = Model(
        input=base_model.input,
        output=base_model.get_layer('activation_40').output
    )

    try:
        generator.model.load_weights(param_path)
        print("Weight load successfully")
    except:
        print("cannot find weight")

    env = TorcsEnv(throttle=True, gear_change=False)

    print("Start driving ...")
    ob = env.reset(relaunch=True)
    feat, aux = get_state(ob, aux_dim, feat_extractor)

    encode = np.zeros((1, encode_dim), dtype=np.float32)
    encode[0, code] = 1
    print "Encode:", encode[0]

    pre_actions = np.load(pre_actions_path)["actions"]

    for i in xrange(MAX_STEP_LIMIT):
        if i < PRE_STEP:
            action = pre_actions[i]
        else:
            action = generator.model.predict([feat, aux, encode])[0]

        ob, reward, done, _ = env.step(action)
        feat, aux = get_state(ob, aux_dim, feat_extractor)

        if i == PRE_STEP:
            print "Start deciding ..."

        print "Step:", i, "DistFromStart:", ob.distFromStart, \
                "TrackPos: %.4f" % ob.trackPos, "Damage:", ob.damage.item(), \
                "Action: %.6f %.6f %.6f" % (action[0], action[1], action[2]), \
                "Speed:", ob.speedX * 200

        if done:
            break

    env.end()
    print("Finish.")


if __name__ == "__main__":
    main()
