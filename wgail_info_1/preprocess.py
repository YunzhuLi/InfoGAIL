from keras.applications.resnet50 import ResNet50
from keras.preprocessing import image
from keras.applications.resnet50 import preprocess_input
from keras.models import Model
import numpy as np
import time
import cv2


def collect_demo(path, num_patch, aux_dim, action_dim):

    for i in xrange(num_patch):
        path_patch = path + str(i) + "/"
        file_name = path_patch + "human_demo.txt"
        raw = open(file_name, 'r').readlines()
        pa = np.zeros(6, dtype=np.float32)

        print "Loading patch %d ..." % i
        for j in xrange(0, len(raw)):
            data = np.array(raw[j].strip().split(" ")).astype(np.float32)
            aux = np.expand_dims(
                np.array([data[1], data[4], data[5], data[6],
                          pa[0], pa[1], pa[2], pa[3], pa[4], pa[5]]),
                axis=0).astype(np.float32)
            action = np.expand_dims(data[aux_dim:], axis=0).astype(np.float32)
            pa[0:3] = pa[3:6]
            pa[3:6] = action[:]

            if j < 120:
                continue
            img_path = path_patch + "capture-" + str(j) + ".png"
            img = image.load_img(img_path)
            img = image.img_to_array(img)
            img = cv2.resize(img, (200, 150))
            img = img[40:, :, :]
            img = np.expand_dims(img, axis=0).astype(np.uint8)

            if j == 120:
                auxs_tmp = aux
                actions_tmp = action
                imgs_tmp = img
            else:
                auxs_tmp = np.concatenate((auxs_tmp, aux), axis=0)
                actions_tmp = np.concatenate((actions_tmp, action), axis=0)
                imgs_tmp = np.concatenate((imgs_tmp, img), axis=0)

        if i == 0:
            auxs = auxs_tmp
            actions = actions_tmp
            imgs = imgs_tmp
        else:
            auxs = np.concatenate((auxs, auxs_tmp), axis=0)
            actions = np.concatenate((actions, actions_tmp), axis=0)
            imgs = np.concatenate((imgs, imgs_tmp), axis=0)

        print "Current total:", imgs.shape, auxs.shape, actions.shape

    print "Images:", imgs.shape, "Auxs:", auxs.shape, "Actions:", actions.shape

    return imgs, auxs, actions


def normalize(x):
    x[:, 0:4] /= 200.
    return x


def main():
    aux_dim = 66
    action_dim = 3
    num_patch = 80
    demo_path = "/home/yunzhu/Desktop/human_low_case_1/demo_"

    imgs, auxs, actions = collect_demo(demo_path, num_patch, aux_dim, action_dim)
    auxs = normalize(auxs)

    np.savez_compressed("/home/yunzhu/Desktop/human_low_case_1/demo.npz",
                        imgs=imgs, auxs=auxs, actions=actions)
    print "Finished."


if __name__ == "__main__":
    main()
