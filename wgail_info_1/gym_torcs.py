import gym
from gym import spaces
import numpy as np
# from os import path
import snakeoil_gym as snakeoil
import numpy as np
import copy
import collections as col
import os
import time


class TorcsEnv:
    terminal_judge_start = 100  # If after 100 timestep still no progress, terminated
    termination_limit_progress = 2  # [km/h], episode terminates if car is running slower than this limit
    termination_limit_stuck_cnt = 50
    default_speed = 50

    initial_reset = True

    def __init__(self, throttle=False, gear_change=False):
        self.throttle = throttle
        self.gear_change = gear_change
        self.stuck_cnt = 0

        self.pre_action_0 = np.zeros(3, dtype=np.float32)
        self.pre_action_1 = np.zeros(3, dtype=np.float32)

        print("launch torcs")
        os.system('pkill torcs')
        time.sleep(0.5)
        os.system('torcs -nofuel -nolaptime &')
        time.sleep(0.5)
        os.system('sh autostart.sh')
        time.sleep(0.5)

        if throttle is False:
            self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(1,))
        else:
            self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(2,))

        high = np.array([1., np.inf, np.inf, np.inf, 1., np.inf, 1., np.inf, 255])
        low = np.array([0., -np.inf, -np.inf, -np.inf, 0., -np.inf, 0., -np.inf, 0])
        self.observation_space = spaces.Box(low=low, high=high)

    def step(self, u):
        # convert thisAction to the actual torcs actionstr
        client = self.client

        this_action = self.agent_to_torcs(u)

        # Apply Action
        action_torcs = client.R.d

        # Steering
        action_torcs['steer'] = this_action['steer']  # in [-1, 1]

        #  Simple Automatic Throttle Control by Snakeoil
        if self.throttle is False:
            target_speed = self.default_speed
            if client.S.d['speedX'] < target_speed - (client.R.d['steer'] * 50):
                client.R.d['accel'] += .01
            else:
                client.R.d['accel'] -= .01

            if client.R.d['accel'] > 0.2:
                client.R.d['accel'] = 0.2

            if client.S.d['speedX'] < 10:
                client.R.d['accel'] += 1 / (client.S.d['speedX'] + .1)

            # Traction Control System
            if ((client.S.d['wheelSpinVel'][2] + client.S.d['wheelSpinVel'][3]) -
                (client.S.d['wheelSpinVel'][0] + client.S.d['wheelSpinVel'][1]) > 5):
                action_torcs['accel'] -= .2
        else:
            action_torcs['accel'] = this_action['accel']
            action_torcs['brake'] = this_action['brake']

        #  Automatic Gear Change by Snakeoil
        if self.gear_change is True:
            action_torcs['gear'] = this_action['gear']
        else:
            #  Automatic Gear Change by Snakeoil is possible
            action_torcs['gear'] = 1
            if self.throttle:
                if client.S.d['speedX'] > 80:
                    action_torcs['gear'] = 2
                if client.S.d['speedX'] > 110:
                    action_torcs['gear'] = 3
                if client.S.d['speedX'] > 140:
                    action_torcs['gear'] = 4
                if client.S.d['speedX'] > 170:
                    action_torcs['gear'] = 5
                if client.S.d['speedX'] > 200:
                    action_torcs['gear'] = 6
        # Save the privious full-obs from torcs for the reward calculation
        obs_pre = copy.deepcopy(client.S.d)
        self.pre_action_0 = self.pre_action_1
        self.pre_action_1 = np.array([action_torcs['steer'],
                                      action_torcs['accel'],
                                      action_torcs['brake']])

        # One-Step Dynamics Update #################################
        # Apply the Agent's action into torcs
        client.respond_to_server()
        # Get the response of TORCS
        client.get_servers_input()
        client.get_servers_input_tcp()

        # Get the current full-observation from torcs
        obs = client.S.d

        # Make an obsevation from a raw observation vector from TORCS
        self.observation = self.make_observation(obs)

        # Reward setting Here #######################################
        # direction-dependent positive reward
        track = np.array(obs['track'])
        trackPos = np.array(obs['trackPos'])
        sp = np.array(obs['speedX'])
        damage = np.array(obs['damage'])
        rpm = np.array(obs['rpm'])

        reward = 0

        # collision detection
        if obs['damage'] - obs_pre['damage'] > 0:
            print("collide")
            reward = -200
            episode_terminate = True
            client.R.d['meta'] = True

        # Episode is terminated if the car is out of track
        if (abs(track.any()) > 1 or abs(trackPos) > 1):
            print("Out of track")
            reward = -200
            episode_terminate = True
            client.R.d['meta'] = True

        # Episode is terminated if the agent stuck for a long time
        progress = sp * np.cos(obs['angle'])
        if self.terminal_judge_start < self.time_step:
            if progress < self.termination_limit_progress:
                self.stuck_cnt += 1
                if self.stuck_cnt > self.termination_limit_stuck_cnt:
                    print("No progress")
                    reward = -200
                    episode_terminate = True
                    client.R.d['meta'] = True
            else:
                self.stuck_cnt = 0

        # Episode is terminated if the agent runs backward
        if np.cos(obs['angle']) < 0:
            print("Run backward")
            reward = -200
            episode_terminate = True
            client.R.d['meta'] = True

        if client.R.d['meta'] is True: # Send a reset signal
            client.respond_to_server()

        self.time_step += 1

        return self.get_obs(), reward, client.R.d['meta'], {}

    def reset(self, relaunch=False):
        #print("Reset")

        self.time_step = 0
        self.stuck_cnt = 0

        if self.initial_reset is not True:
            self.client.R.d['meta'] = True
            self.client.respond_to_server()

            ## TENTATIVE. Restarting TORCS every episode suffers the memory leak bug!
            if relaunch is True:
                self.reset_torcs()
                print("### TORCS is RELAUNCHED ###")

        # Modify here if you use multiple tracks in the environment
        self.client = snakeoil.Client(p=3001)  # Open new UDP in vtorcs
        self.client.MAX_STEPS = np.inf

        client = self.client
        client.get_servers_input()  # Get the initial input from torcs
        client.get_servers_input_tcp()

        obs = client.S.d  # Get the current full-observation from torcs
        self.observation = self.make_observation(obs)

        self.initial_reset = False
        return self.get_obs()

    def end(self):
        os.system('pkill torcs')

    def get_obs(self):
        return self.observation

    def reset_torcs(self):
        #print("relaunch torcs")
        os.system('pkill torcs')
        time.sleep(0.5)
        os.system('torcs -nofuel -nolaptime &')
        time.sleep(0.5)
        os.system('sh autostart.sh')
        time.sleep(0.5)

    def agent_to_torcs(self, u):
        torcs_action = {'steer': u[0]}

        if self.throttle is True:  # throttle action is enabled
            torcs_action.update({'accel': u[1]})
            torcs_action.update({'brake': u[2]})

        if self.gear_change is True: # gear change action is enabled
            torcs_action.update({'gear': int(u[3])})

        return torcs_action

    def make_observation(self, raw_obs):
        names = ['focus', 'speedX', 'speedY', 'speedZ', 'angle', 'damage',
                 'opponents', 'rpm', 'track', 'trackPos', 'wheelSpinVel',
                 'img', 'pre_action_0', 'pre_action_1', 'distFromStart']
        Observation = col.namedtuple('Observaion', names)

        return Observation(focus = np.array(raw_obs['focus'], dtype =
                                            np.float32) / 200.,
                           speedX = np.array(raw_obs['speedX'],
                                             dtype=np.float32) / 200.,
                           speedY = np.array(raw_obs['speedY'], dtype =
                                             np.float32) / 200.,
                           speedZ = np.array(raw_obs['speedZ'],
                                             dtype=np.float32) / 200.,
                           angle = np.array(raw_obs['angle'], dtype =
                                            np.float32) / 3.1416,
                           damage = np.array(raw_obs['damage'] / 200., dtype = np.float32),
                           opponents = np.array(raw_obs['opponents'],
                                                dtype = np.float32) / 200.,
                           rpm = np.array(raw_obs['rpm'], dtype =
                                          np.float32) / 1000,
                           track = np.array(raw_obs['track'], dtype =
                                            np.float32) / 200.,
                           trackPos = np.array(raw_obs['trackPos'], dtype=np.float32),
                           wheelSpinVel = np.array(raw_obs['wheelSpinVel'],
                                                   dtype=np.float32) / 200.,
                           img=raw_obs['img'],
                           pre_action_0 = self.pre_action_0,
                           pre_action_1 = self.pre_action_1,
                           distFromStart = raw_obs['distFromStart'])
