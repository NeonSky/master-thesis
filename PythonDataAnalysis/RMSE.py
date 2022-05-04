import os
import numpy as np
from sklearn.metrics import mean_squared_error
from math import sqrt, sin
from matplotlib import pyplot
from PIL import Image

# Dir folder should contain subfolders for each seed with an ewave texture data file per frame.
dir = ""

black = (0.0, 0.0, 0.0)
red = (204/255, 0, 0)
orange = (1.0, 102/255, 0)
yellow = (204/255, 204/255, 0)
blue = (0, 0, 204/255)

numSeeds = 0
if __name__ == '__main__':
    files = []
    all_files_by_seed = {}
    numSeeds = len(os.listdir(dir))
    print("There are " + str(numSeeds) + " seeds.")
    for seed_dir in os.listdir(dir):
        print("Seed: " + seed_dir)
        seed_files = {}
        for h_file in os.listdir(dir + "/" + seed_dir):
            print("File: " + h_file)
            full_file_path = dir + "/" + seed_dir + "/" + h_file
            with open(full_file_path, "r") as h_filestream:
                file = []
                for line in h_filestream:
                    if "Test" in line:
                        continue
                    currentline_strings = line.split(",")
                    currentline_strings_only_values = [h_str for h_str in currentline_strings if "r:" in h_str]
                    currentline = [float(h_str[3:]) for h_str in currentline_strings_only_values]
                    file.append(np.array(currentline))
                seed_files[h_file] = file
            h_filestream.close()
        all_files_by_seed[seed_dir] = seed_files

    rmse_by_frame_art1 = [0] * 20
    rmse_by_frame_art2 = [0] * 20
    rmse_by_frame_art3 = [0] * 20
    rmse_by_frame_org  = [0] * 20

    for seed, files in all_files_by_seed.items():
        for filename, file in files.items():
            type = filename[:4]
            frame = filename[5:-4]
            if type == "ART0":
                continue
            reference_file = "ART0_" + filename[5:]
            if "Organic" in filename:
                reference_file = "ART0_" + filename[7:]
                frame = filename[7:-4]
            rmse = sqrt(mean_squared_error(files[reference_file], file))
            if type == "ART1":
                rmse_by_frame_art1[int((int(frame) / 50) - 1)] += rmse
            if type == "ART2":
                rmse_by_frame_art2[int((int(frame) / 50) - 1)] += rmse
            if type == "ART3":
                rmse_by_frame_art3[int((int(frame) / 50) - 1)] += rmse
            if "Organic" in filename:
                rmse_by_frame_org[int((int(frame) / 50) - 1)] += rmse
        breakpoint = 2


    for rmse in rmse_by_frame_art1:
        print("Total RMSE for frame " + ": " + str(rmse))

    rmse_by_frame_art1 = np.array(rmse_by_frame_art1) / numSeeds
    rmse_by_frame_art2 = np.array(rmse_by_frame_art2) / numSeeds
    rmse_by_frame_art3 = np.array(rmse_by_frame_art3) / numSeeds
    rmse_by_frame_org = np.array(rmse_by_frame_org) / numSeeds

    xPoints = [x * 50 for x in range(1, 21)]
    pyplot.title("eWave simulation texture RMSE")
    pyplot.xlabel("Frame")
    pyplot.ylabel("RMSE")
    pyplot.plot(xPoints, rmse_by_frame_art1, "-o", color=yellow, label="Delay 1")
    pyplot.plot(xPoints, rmse_by_frame_art2, "-o", color=orange, label="Delay 2")
    pyplot.plot(xPoints, rmse_by_frame_art3, "-o", color=red, label="Delay 3")
    pyplot.plot(xPoints, rmse_by_frame_org, "-o", color=blue, label="Organic")
    pyplot.legend(loc="upper left")
    pyplot.show()
