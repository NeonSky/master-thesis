import json
import os
import numpy as np
from matplotlib import pyplot as plt


#dir = '../UnrealMasterThesis/SavedBoatData/MAIN_LatencyFix/OneBoat'
dir = '../UnrealMasterThesis/SavedBoatData/MAIN_post_normals/24d9ecfbd10473a8fddcfd5789d2c348af83ba53/One_boat'

avg_over_multiple_seeds = True

# Set 'dir' to a folder with recorded boat data
# If more than one recording per boat type is present, only the first file for each type will be used
# Assumes file names contains boat type CPU / GPU / ART0 / ART1 / ART2 / ART3

black = (0.0, 0.0, 0.0)
red = (204/255, 0, 0)
orange = (1.0, 102/255, 0)
yellow = (204/255, 204/255, 0)

def getColor(boat_type):
    global reds
    if boat_type == 'ART0':
        return black
    if boat_type == 'ART1':
        return yellow
    if boat_type == 'ART2':
        return orange
    if boat_type == 'ART3':
        return red

def extractPositionData(in_json_list):
    result = []
    for frameData_str in in_json_list:
        frameData = json.loads(frameData_str)
        pos = np.array([frameData['position_x'], frameData['position_y']])
        result.append(pos)
    return result


def getErrorMagnitudes(referece_data, other_data):
    error_magnitudes = []
    if not len(other_data) == 0:
        for i in range(len(referece_data)):
            b1 = referece_data[i]
            b2 = other_data[i]
            diff = b1-b2
            mag = np.sqrt(diff.dot(diff))
            error_magnitudes.append(mag)
    return error_magnitudes


def getPlotLimits(pos_data_1, pos_data_2, pos_data_3):
    min_x = min_y = float('inf')
    max_x = max_y = float('-inf')

    all_positions = [pos for boat_type, positions in pos_data_1.items() for pos in positions]
    all_positions.extend([pos for boat_type, positions in pos_data_2.items() for pos in positions])
    all_positions.extend([pos for boat_type, positions in pos_data_3.items() for pos in positions])

    for pos in all_positions:
        if pos[0] < min_x:
            min_x = pos[0]
        if pos[1] < min_y:
            min_y = pos[1]
        if pos[0] > max_x:
            max_x = pos[0]
        if pos[1] > max_y:
            max_y = pos[1]

    diff_x = abs(max_x - min_x)
    diff_y = abs(max_y - min_y)
    # Add 10% to the limit to give a small margin
    result = [min_x - (diff_x / 10), max_x + (diff_x / 10), min_y - (diff_y / 10), max_y + (diff_y / 10)]
    return result


def getJSONdataFromAllFilesInDir(directory):
    json_data = {}
    for file_name in os.listdir(directory):
        file_path = os.path.join(directory, file_name)
        file = open(file_path)
        if 'CPU' in file_path:
            if 'CPU' not in json_data:
                json_data['CPU'] = json.load(file)
        elif 'GPU' in file_path:
            if 'GPU' not in json_data:
                json_data['GPU'] = json.load(file)
        elif 'ART0' in file_path:
            if 'ART0' not in json_data:
                json_data['ART0'] = json.load(file)
        elif 'ART1' in file_path:
            if 'ART1' not in json_data:
                json_data['ART1'] = json.load(file)
        elif 'ART2' in file_path:
            if 'ART2' not in json_data:
                json_data['ART2'] = json.load(file)
        elif 'ART3' in file_path:
            if 'ART3' not in json_data:
                json_data['ART3'] = json.load(file)
        else:
            print('File names should contain boat type. CPU, GPU, ART0 - ART3')
        file.close()
    return json_data

def createPathsPlot(title, pos_data, seed_dir, show = False):
    global dir
    saveFilePath = dir + "/SavedPlots/"
    # Plot boat travel paths for 2 boats
    plt.title(title)
    plt.xlabel("x (m)")
    plt.ylabel("y (m)")
    plt.grid(visible=True)
    plotLimits = getPlotLimits(position_data, position_data_2, position_data_3)
    plt.xlim(plotLimits[0], plotLimits[1])
    plt.ylim(plotLimits[2], plotLimits[3])
    plt.plot(0, 0, "x", label="Origin", color=black)
    for boat in pos_data:
        for boat_type, p in boat.items():
            if boat_type == 'CPU':
                continue
            positions = np.array(p)
            plt.plot(positions[:, 0], positions[:, 1], label=boat_type.replace("ART", "Delay "), color=getColor(boat_type))
    plt.legend(loc="upper left")
    if seed_dir == "3" or seed_dir == "5" or seed_dir == "32" or seed_dir == "53":
        #plt.axis('scaled')
        plt.show()
    else:
        plt.axis('scaled')
        #plt.gca().set_aspect('equal', adjustable='box')
        plt.savefig(saveFilePath + "PathsPlot_" + seed_dir + ".png")
        plt.clf()

def createDifferenceMagnitudePlot(title, reference, difference_magnitudes, numFrames, seed_dir, show = False):
    global dir
    saveFilePath = dir + "/SavedPlots/"
    x = np.arange(0, numFrames)
    plt.title(title)
    plt.xlabel("Frame")
    plt.ylabel("Magnitude of Position Deviation (m)")
    for boat_type, diff in difference_magnitudes.items():
        if boat_type == 'CPU':
            continue
        if len(diff) > 0:
            plt.plot(x, diff, label=boat_type.replace("ART", "Delay "), color=getColor(boat_type))
    plt.legend(loc="upper left")
    if False:
        plt.show()
    else:
        plt.savefig(saveFilePath + "DiffPlot" + seed_dir + ".png")
        plt.clf()
        for boat_type, diff in difference_magnitudes.items():
            if boat_type == 'CPU':
                continue
            if len(diff) > 0:
                plt.plot(x[:200], diff[:200], label=boat_type.replace("ART", "Delay "), color=getColor(boat_type))
        plt.title(title)
        plt.xlabel("Frame")
        plt.ylabel("Magnitude of Position Deviation (m)")
        plt.legend(loc="upper left")
        plt.savefig(saveFilePath + "DiffPlot_zoom" + seed_dir + ".png")
        plt.clf()

def createCumulativeDiffPlot(title, reference, difference_magnitudes, numFrames, seed_dir, show = False):
    global dir
    saveFilePath = dir + "/SavedPlots/"
    x = np.arange(0, numFrames)
    plt.title(title)
    plt.xlabel("Frame")
    plt.ylabel("Cumulative Magnitude of Position Deviation (m)")
    for boat_type, diff in difference_magnitudes.items():
        if boat_type == 'CPU':
            continue
        cumulative = [0] * numFrames
        cumulative[0] = diff[0]
        for i in range(1, numFrames):
            cumulative[i] = diff[i] + cumulative[i - 1]
        if len(diff) > 0:
            plt.plot(x, cumulative, label=boat_type.replace("ART", "Delay "), color=getColor(boat_type))
    plt.legend(loc="upper left")
    if False:
        plt.show()
    else:
        plt.savefig(saveFilePath + "CumulativeDiffPlot" + seed_dir + ".png")
        plt.clf()
        for boat_type, diff in difference_magnitudes.items():
            if boat_type == 'CPU':
                continue
            cumulative = [0] * numFrames
            cumulative[0] = diff[0]
            for i in range(1, numFrames):
                cumulative[i] = diff[i] + cumulative[i - 1]
            if len(diff) > 0:
                plt.plot(x[:200], cumulative[:200], label=boat_type.replace("ART", "Delay "), color=getColor(boat_type))
        plt.title(title)
        plt.xlabel("Frame")
        plt.ylabel("Cumulative Magnitude of Position Deviation (m)")
        plt.legend(loc="upper left")
        plt.savefig(saveFilePath + "CumulativeDiffPlot_zoom" + seed_dir + ".png")
        plt.clf()



if __name__ == '__main__':
    if avg_over_multiple_seeds:
        diff_per_seed = {}
        frame_of_deviation_per_seed = {}
        frame_of_1m_deviaion_per_seed = {}
        for seed_dir in os.listdir(dir):
            if seed_dir == "SavedPlots":
                continue
            frame_of_1m_deviaion_per_seed[seed_dir] = {}
            frame_of_deviation_per_seed[seed_dir] = {}
            print("Seed: " + seed_dir)
            json_data = getJSONdataFromAllFilesInDir(dir + '/' + seed_dir)
            # Extract Position data into arrays
            boat_1 = boat_2 = boat_3 = False
            position_data = {}
            position_data_2 = {}
            position_data_3 = {}
            numFrames = 0
            for boat_type, boats in json_data.items():
                position_data[boat_type] = extractPositionData(boats['boat_0'])
                position_data_2[boat_type] = extractPositionData(boats['boat_1'])
                position_data_3[boat_type] = extractPositionData(boats['boat_2'])
                numFrames = len(position_data[boat_type])
                if len(position_data_2[boat_type]) > 0:
                    boat_2 = True
                if len(position_data_3[boat_type]) > 0:
                    boat_3 = True

            if not boat_2 and not boat_3:
                # The data in this folder only contains one simultaneous boat.
                # Assuming a plot of different boat types is of interest.
                diff_magnitudes = {}
                reference_positions = position_data['ART0']
                for boat_type, positions in position_data.items():
                    if boat_type == 'ART0':  # GPU is the reference boat, no need to compare GPU vs GPU.
                        continue
                    diff_magnitudes[boat_type] = getErrorMagnitudes(reference_positions, positions)

                diff_per_seed[seed_dir] = diff_magnitudes
                createDifferenceMagnitudePlot("Error compared to a boat with a 0 frame delay", "Delay 0", diff_magnitudes, numFrames, seed_dir)
                createPathsPlot("Paths traveled by boats with different frame delay", [position_data], seed_dir)
                createCumulativeDiffPlot("Cumulative error compared to a boat with a 0 frame delay", "Delay 0", diff_magnitudes, numFrames, seed_dir)

                for boat_type, diffs in diff_magnitudes.items():
                    for i in range(numFrames):
                        diff_at_frame_i = diffs[i]
                        # TODO: frame of first deviation as well
                        if diff_at_frame_i > 1.0:
                            frame_of_1m_deviaion_per_seed[seed_dir][boat_type] = i
                            break
                    for i in range(numFrames):
                        diff_at_frame_i = diffs[i]
                        # TODO: frame of first deviation as well
                        if diff_at_frame_i > 0.0:
                            frame_of_deviation_per_seed[seed_dir][boat_type] = i
                            break
            else:
                # At least 2 simultaneous boats.
                # Assuming a plot of comparing simultaneous boats is of interest
                diff_magnitudes = {}
                reference_positions = list(position_data.values())[0]  # reference boat is boat 1
                boat_2_positions = list(position_data_2.values())[0]
                boat_3_positions = list(position_data_3.values())[0]
                diff_magnitudes['Boat 2'] = getErrorMagnitudes(reference_positions, boat_2_positions)
                diff_magnitudes['Boat 3'] = getErrorMagnitudes(reference_positions, boat_3_positions)
                createDifferenceMagnitudePlot("Error, simultaneous boats", "Boat 1", diff_magnitudes, numFrames)
                createPathsPlot("Paths, multiple simultaneous boats", [position_data, position_data_2, position_data_3])


        all_diffs_per_type = {}
        diff_avg_per_type = {}
        for seed, diff_dict in diff_per_seed.items():
            if not 'ART1' in all_diffs_per_type:
                all_diffs_per_type['ART1'] = []
            all_diffs_per_type['ART1'].append(diff_dict['ART1'])
            if not 'ART2' in all_diffs_per_type:
                all_diffs_per_type['ART2'] = []
            all_diffs_per_type['ART2'].append(diff_dict['ART2'])
            if not 'ART3' in all_diffs_per_type:
                all_diffs_per_type['ART3'] = []
            all_diffs_per_type['ART3'].append(diff_dict['ART3'])

        for boat_type, diff_lists in all_diffs_per_type.items():
            diff_sum = np.array(diff_lists).sum(axis=0)
            diff_avg = diff_sum / 20.0
            diff_avg_per_type[boat_type] = diff_avg

        #def createCumulativeDiffPlot(title, reference, difference_magnitudes, numFrames, show = False):
        createDifferenceMagnitudePlot("Average position error over 20 seeds", "Delay 0", diff_avg_per_type, 1000, "Average", True)
        createCumulativeDiffPlot("Average cumulative position error over 20 seeds,", "Delay 0", diff_avg_per_type, 1000, "Average", True)


        first_to_deviate_1m_count = [0, 0, 0]
        for seed, boat_types in frame_of_deviation_per_seed.items():
            print("Seed: " + seed)
            for boat_type, frame in boat_types.items():
                print("\tBoat_type " + boat_type + " deviated at frame: " + str(frame))

        print("\n.\n.\n.\n.\n.\n.________1m________\n")
        for seed, boat_types in frame_of_1m_deviaion_per_seed.items():
            print("Seed: " + seed)
            for boat_type, frame in boat_types.items():
                print("\tBoat_type " + boat_type + " deviated at frame: " + str(frame))




