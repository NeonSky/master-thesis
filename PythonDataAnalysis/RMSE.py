import os
import numpy as np
from sklearn.metrics import mean_squared_error
from math import sqrt, sin
from matplotlib import pyplot
from PIL import Image

dir = '../UnrealMasterThesis/SavedBoatData/eWaveTextures/'
# jag vill ha dom i seed / boat type / frame.txt

black = (0.0, 0.0, 0.0)
red = (204/255, 0, 0)
orange = (1.0, 102/255, 0)
yellow = (204/255, 204/255, 0)

if __name__ == '__main__':
    files = []
    all_files_by_seed = {}
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
                #boat_type = h_file[:4]
                #frameNr = int(h_file[5:-4])
                #print("Boat type: " + boat_type + "   frame nr: " + str(frameNr))
                seed_files[h_file] = file
            h_filestream.close()
        all_files_by_seed[seed_dir] = seed_files

    rmse_by_frame_art1 = [0] * 20
    rmse_by_frame_art2 = [0] * 20
    rmse_by_frame_art3 = [0] * 20

    for seed, files in all_files_by_seed.items():
        for filename, file in files.items():

            #mat = np.reshape(file, (256, 256))
            #img = Image.fromarray(np.uint8(mat * 255), 'L')
            #img.show()

            type = filename[:4]
            frame = filename[5:-4]
            if type == "ART0":
                continue
            reference_file = "ART0_" + filename[5:]
            rmse = sqrt(mean_squared_error(files[reference_file], file))
            if type == "ART1":
                rmse_by_frame_art1[int((int(frame) / 50) - 1)] += rmse
            if type == "ART2":
                rmse_by_frame_art2[int((int(frame) / 50) - 1)] += rmse
            if type == "ART3":
                rmse_by_frame_art3[int((int(frame) / 50) - 1)] += rmse


    for rmse in rmse_by_frame_art1:
        print("Total RMSE for frame " + ": " + str(rmse))
            #print("Type: " + filename[:4] + "   frameNr: " + filename[5:-4] + "   Ref file: " + reference_file + "   RMSE: " + str(rmse))

    rmse_by_frame_art1 = np.array(rmse_by_frame_art1) / 20.0 # TODO replace 20 with a num seeds variable
    rmse_by_frame_art2 = np.array(rmse_by_frame_art2) / 20.0
    rmse_by_frame_art3 = np.array(rmse_by_frame_art3) / 20.0

    xPoints = [x * 50 for x in range(1, 21)]
    pyplot.title("eWave simulation texture RMSE")
    pyplot.xlabel("Frame")
    pyplot.ylabel("RMSE")
    pyplot.plot(xPoints, rmse_by_frame_art1, "-o", color=yellow, label="Delay 1")
    pyplot.plot(xPoints, rmse_by_frame_art2, "-o", color=orange, label="Delay 2")
    pyplot.plot(xPoints, rmse_by_frame_art3, "-o", color=red, label="Delay 3")
    pyplot.legend(loc="upper left")
    pyplot.show()
