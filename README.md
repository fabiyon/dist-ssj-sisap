# dist-ssj-sisap

This is the code to the experiments to the paper "Scaling Up Set Similarity Joins Using A Cost-Based Distributed-Parallel Framework" accepted for SISAP 2021: http://sisap.org/2021/papers.html

The code implements an extension of our multicore SSJ on one node (https://github.com/fabiyon/ssj-sisap) using additional optimization parameters indexLengths, probeLengths, mod, and modGroup.

## Compilation
The code requires the packages libboost-all-dev libgflags-dev libgoogle-glog-dev to compile. It compiles with make.

## Input File Requirements
* First line: numberOfRecords numberOfDistinctTokens totalNumberOfTokens
* Record line: numberOfTokens TAB tokensCommaSeparated
* The records must be sorted in ascending lengths
* The tokens must be integers starting from 0 and be dense

## Usage
./mssj -method allph -input_file path_to_file -threshold threshold_between_.5_and_.99 -threads 24 -pos_filter 1 -indexLengths comma_separated_integers -probeLengths comma_separated_integers -mod modulo -modgroup modgroup

Example:
./mssj -method allph -input_file ../data/AOL-100 -threshold .9 -threads 24 -pos_filter 1 -indexLengths 1,2,3,4 -probeLengths 3,4,5 -m 3 -modgroup 2

# Optimizer

The file optimizer.php exemplarily shows how to obtain the execution parameters required to execute the worker for BPOSx10. The script parameters are described in the PHP file.

## Usage
php optimizer.php (PHP CLI required)

# Finding Parameters

The file findingParameters.php exemplarily shows how to obtain the optimization parameters for AOLx50 and BPOSx50.

## Usage
php findingParameters.php (PHP CLI required)

# Test Data
We included the statistics for the x10 and x50 datasets in /data, which are necessary for the optimizer.
