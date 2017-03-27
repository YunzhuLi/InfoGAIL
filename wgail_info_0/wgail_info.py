from gym_torcs import TorcsEnv
import numpy as np
import argparse
import time
from keras.models import model_from_json, Model
from keras.models import Sequential
from keras.layers.core import Dense, Dropout, Activation, Flatten
from keras.optimizers import Adam
import tensorflow as tf
import json

from models import TRPOAgent


def playGame(finetune=0):

    demo_dir = "/home/yunzhu/Desktop/human_0/"
    param_dir = "/home/yunzhu/Desktop/wgail_info_params_0/"
    pre_actions_path = "/home/yunzhu/Desktop/human_0/pre_actions.npz"
    feat_dim = [7, 13, 1024]
    img_dim = [50, 50, 3]
    aux_dim = 10
    encode_dim = 2
    action_dim = 3

    np.random.seed(1024)

    config = tf.ConfigProto()
    config.gpu_options.allow_growth = True
    sess = tf.Session(config=config)
    from keras import backend as K
    K.set_session(sess)

    # initialize the env
    env = TorcsEnv(throttle=True, gear_change=False)

    # define the model
    pre_actions = np.load(pre_actions_path)["actions"]
    agent = TRPOAgent(env, sess, feat_dim, aux_dim, encode_dim, action_dim,
                      img_dim, pre_actions)

    # Load expert (state, action) pairs
    demo = np.load(demo_dir + "demo.npz")

    # Now load the weight
    print("Now we load the weight")
    try:
        if finetune:
            agent.generator.load_weights(
                param_dir + "params_0/generator_model_37.h5")
            agent.discriminator.load_weights(
                param_dir + "params_0/discriminator_model_37.h5")
            agent.baseline.model.load_weights(
                param_dir + "params_0/baseline_model_37.h5")
            agent.posterior.load_weights(
                param_dir + "params_0/posterior_model_37.h5")
            agent.posterior_target.load_weights(
                param_dir + "params_0/posterior_target_model_37.h5")
        else:
            agent.generator.load_weights(
                param_dir + "params_bc/params_3/generator_bc_model.h5")
        print("Weight load successfully")
    except:
        print("Cannot find the weight")

    print("TORCS Experiment Start.")
    agent.learn(demo)

    print("Finish.")


if __name__ == "__main__":
    playGame()
