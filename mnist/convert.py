
from mnist import MNIST
import numpy as np
from skimage.transform import resize


def reshape_image(image, shape):
    return resize(image, shape)


def normalize_image(image):
    vmin = image.min()
    vmax = image.max()
    return (image - vmin) / (vmax - vmin)


def process_dataset(images):
    arr = []
    for image in images:
        image_resized = reshape_image(np.array(image).reshape((28, 28)), (11, 11))
        image_normed = normalize_image(image_resized)

        arr.append(image_normed.reshape(-1).tolist())

    return arr


def write_to_file(images, labels, filename, cat_filter=None):
    with open(filename, 'w') as f:
        for image, label in zip(images, labels):
            if cat_filter is not None:
                if label not in cat_filter:
                    continue

            f.write("%d " % label)
            for v in image:
                f.write("%f " % v)

            f.write("\n")


if __name__ == '__main__':

    mndata = MNIST(r'./raw')

    training_images, training_labels = mndata.load_training()

    write_to_file(process_dataset(training_images), training_labels.tolist(), r'./121-size-012-cat/train.txt', [0, 1, 2])

    testing_images, testing_labels = mndata.load_testing()

    write_to_file(process_dataset(testing_images), testing_labels.tolist(), r'./121-size-012-cat/test.txt', [0, 1, 2])
