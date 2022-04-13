import json
import os
import numpy as np
from matplotlib import pyplot as plt

dir = '../UnrealMasterThesis/SavedBoatData/Pre_PR_Test'

# Set 'dir' to a folder with recorded boat data
# If more than one recording per boat type is present, only the first file for each type will be used
# Assumes file names contains boat type CPU / GPU / ART0 / ART1 / ART2 / ART3


def extractPositionData(in_json_dict):
    boat_1_positions = []
    for frameData_str in in_json_dict['boatPositions']:
        frameData = json.loads(frameData_str)
        pos = np.array([frameData['position_x'],
                         frameData['position_y'],
                         frameData['position_z']])
        boat_1_positions.append(pos)
    return boat_1_positions


def getErrorMagnitudes(referece_data, other_data):
    error_magnitudes = []
    for i in range(len(referece_data)):
        b1 = referece_data[i]
        b2 = other_data[i]
        diff = b1-b2
        mag = np.sqrt(diff.dot(diff))
        error_magnitudes.append(mag)
    return error_magnitudes


def getPlotLimits(datasets):
    min_x = min_y = float('inf')
    max_x = max_y = float('-inf')
    for key, dataset in datasets.items():
        for pos in dataset:
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


if __name__ == '__main__':
    json_data = getJSONdataFromAllFilesInDir(dir)

    # Extract Position data into arrays
    position_data = {}
    for boat_type, data in json_data.items():
        position_data[boat_type] = extractPositionData(data)

    # Get magnitude or diff compared to GPU boat
    diff_magnitudes = {}
    reference_positions = position_data['GPU']
    for boat_type, positions in position_data.items():
        if boat_type == 'GPU':
            continue
        diff_magnitudes[boat_type] = getErrorMagnitudes(reference_positions, positions)

    # Plot difference magnitudes
    x = np.arange(0, 600)
    plt.title("Boat Position Difference")
    plt.xlabel("frame")
    plt.ylabel("difference magnitudes")
    for boat_type, diff in diff_magnitudes.items():
        plot_label = 'GPU vs ' + boat_type
        plt.plot(x, diff, label=plot_label)
    plt.legend(loc="upper left")
    plt.show()

    # Plot boat travel paths
    plt.title("Boat Paths")
    plt.xlabel("x")
    plt.ylabel("y")
    position_data_2D = {}
    for boat_type, positions in position_data.items():
        position_data_2D[boat_type] = np.array([[x, y] for (x, y, z) in positions])

    for boat_type, positions_2D in position_data_2D.items():
        plt.plot(positions_2D[:, 0], positions_2D[:, 1], label=boat_type)

    plt.grid(visible=True)
    plotLimits = getPlotLimits(position_data)
    plt.xlim(plotLimits[0], plotLimits[1])
    plt.ylim(plotLimits[2], plotLimits[3])
    plt.legend(loc="upper left")
    plt.show()
