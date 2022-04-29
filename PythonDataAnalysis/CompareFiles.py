import os

directory = '../UnrealMasterThesis/SavedBoatData\MAIN_post_normals/24d9ecfbd10473a8fddcfd5789d2c348af83ba53/ScreenShot/Seed_11'

filePaths = []
numFiles = 0
files = {}
for filename in os.listdir(directory):
    fpath = os.path.join(directory, filename)
    numFiles += 1
    file = open(fpath)
    files[fpath] = file.readlines()
    file.close()

scores = {}
for path_1, contents_1 in files.items():
    if path_1 not in scores:
        scores[path_1] = 0

    for path_2, contents_2 in files.items():
        if path_1 == path_2:
            continue
        if path_2 not in scores:
            scores[path_2] = 0

        if contents_1 == contents_2:
            scores[path_1] += 1

consistentReferenceFile = None
inconsistentFiles = []
for file, score in scores.items():
    if score == 0:
        print(file + ' is not matching any other files! INCONSISTENCY!')
        inconsistentFiles.append(file)
    else:
        if consistentReferenceFile == None:
            consistentReferenceFile = files[file]
        print(file + ' is matching ' + str(score) + ' other files.')

for file in inconsistentFiles:
    lineNr = 0
    for line in files[file]:
        refLine = consistentReferenceFile[lineNr]
        if line != refLine:
            #print('------------------------------------------\n' + file + ' has its first inconsistency at line ' + str(lineNr))
            #print('inconsistent line: ' + line)
            #print('reference line: ' + refLine)
            break
        lineNr += 1

