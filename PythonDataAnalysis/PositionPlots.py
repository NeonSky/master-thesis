import json
import os
import numpy as np
from matplotlib import pyplot as plt

dir = '../UnrealMasterThesis/SavedBoatData/TwoBoats_2'

# Set 'dir' to a folder with recorded boat data
# If more than one recording per boat type is present, only the first file for each type will be used
# Assumes file names contains boat type CPU / GPU / ART0 / ART1 / ART2 / ART3


def extractPositionData(in_json_list):
    result = []
    for frameData_str in in_json_list:
        frameData = json.loads(frameData_str)
        pos = np.array([frameData['position_x'],
                         frameData['position_y'],
                         frameData['position_z']])
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
        print(pos)
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

def createPathsPlot(title, pos_data):
    # Plot boat travel paths for 2 boats
    plt.title(title)
    plt.xlabel("x")
    plt.ylabel("y")
    plt.grid(visible=True)
    plotLimits = getPlotLimits(position_data, position_data_2,position_data_3)
    plt.xlim(plotLimits[0], plotLimits[1])
    plt.ylim(plotLimits[2], plotLimits[3])
    for b in pos_data:
        positions_2D = {}
        for boat_type, positions_3D in b.items():
            if len(positions_3D) > 0:
                positions_2D[boat_type] = np.array([[x, y] for (x, y, z) in positions_3D])
        for lbl, positions in positions_2D.items():
            plt.plot(positions[:, 0], positions[:, 1], label=lbl)
    plt.legend(loc="upper left")
    plt.show()

def createDifferenceMagnitudePlot(title, reference, difference_magnitudes, numFrames):
    x = np.arange(0, numFrames)
    plt.title(title)
    plt.xlabel("Frame")
    plt.ylabel("Position difference magnitude")
    for boat_type, diff in difference_magnitudes.items():
        if len(diff) > 0:
            plot_label = reference + ' vs ' + boat_type
            plt.plot(x, diff, label=plot_label)
    plt.legend(loc="upper left")
    plt.show()



if __name__ == '__main__':
    json_data = getJSONdataFromAllFilesInDir(dir)
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
        reference_positions = position_data['GPU']
        for boat_type, positions in position_data.items():
            if boat_type == 'GPU':  # GPU is the reference boat, no need to compare GPU vs GPU.
                continue
            diff_magnitudes[boat_type] = getErrorMagnitudes(reference_positions, positions)
        createDifferenceMagnitudePlot("Error, different boat types", "GPU", diff_magnitudes, numFrames)
        createPathsPlot("One player, multiple boat types", [position_data])
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