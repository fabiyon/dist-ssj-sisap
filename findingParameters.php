<?php

$datapath = "./data/"; // we need the statistics files in this folder, i. e., BPOS-50-stat

$datasetArr = ["AOL-10"]; // the employed datasets
$thetaArr = [0.6]; // thresholds

$startTotalNodes = 16; // seed parameter t
$maxTotalNodes = 64; // max. number of nodes to consider (=max. recursion if no suitable parameter combination could be found)
$minMod = 1; // seed modulo
$maxMem = 28; // in GB for a 32GB machine considering 4GB overhead
$showDetails = true; // show detailed information for every iteration. When set to false, only output the optimal parameters.

// $resFile = $respath.'results.csv';
// if (!file_exists($resFile)) {
//     $fp = fopen($resFile, 'w');
//     fputcsv($fp, ['dataset', 'theta','numberNodes', 'node', 'costs', 'runtime', 'candidates', 'results', 'mod', 'modgroup']);
// } else {
//     $fp = fopen($resFile, 'a');
// }

foreach ($datasetArr as $dataset) {
    $totalNodes = $startTotalNodes;
    $lengthCounts = [];

    $maxLength = 0;
    $h = fopen($datapath.$dataset.'-stat', 'r');
    while ($data = fgetcsv($h)) {
        $lengthCounts[$data[0]] = $data[1]; // length => count
        if ($maxLength < $data[0]) {
            $maxLength = $data[0];
        }
    }
    fclose($h);
    ksort($lengthCounts);

    $opt = null;

    foreach ($thetaArr as $theta) {
        $optRelation = PHP_INT_MAX;

        // Compute probe lengths for each index length:
        $probeLengths = [];
        foreach ($lengthCounts as $indexLength => $count) {
            $maxProbeLength = min(floor($indexLength / $theta), $maxLength);
            for ($probeLength = $indexLength; $probeLength <= $maxProbeLength; $probeLength++) {
                if (array_key_exists($probeLength, $lengthCounts)) {
                    $probeLengths[$indexLength][] = $probeLength; 
                }
            }
        }

        $sliceCosts = [];
        foreach ($probeLengths as $indexLength => $probeLengthArr) {
            $costs = 0;
            // Berechne zunächst die Summe:
            foreach ($probeLengthArr as $probeLength) {
                $prefixLength = $probeLength - ceil($theta * $probeLength) + 1;
                // indexLength ist ungenau: es muss doch auch der Präfix(indexLength) sein...
                $costs += $prefixLength * $lengthCounts[$probeLength]; 
            }
            $prefixLength = $indexLength - ceil($theta * $indexLength) + 1;
            $sliceCosts[$indexLength] = $prefixLength * $lengthCounts[$indexLength] * $costs;
        }
        arsort($sliceCosts);


        // Begin iteration for optimization:
        $nodes = $totalNodes / $minMod;
        $mod = $minMod;

        while ($totalNodes <= $maxTotalNodes) {
            $minCosts = PHP_INT_MAX;
            $maxCosts = 0;

            if ($showDetails) {
                echo "#==$totalNodes: $dataset n=$nodes m=$mod $theta==\n";
            }

            $currentNode = 0;
            $costsPerNode = [];
            $indexLengthsPerNode = [];
            $probeLengthsPerNode = [];
            for ($i = 0; $i < $nodes; $i++) {
                $costsPerNode[$i] = 0;
                $indexLengthsPerNode[$i] = [];
                $probeLengthsPerNode[$i] = [];
            }

            foreach ($sliceCosts as $indexLength => $costs) {
                $costsPerNode[$currentNode] += $costs / $mod;
                $indexLengthsPerNode[$currentNode][] = $indexLength;
                $probeLengthsPerNode[$currentNode] = array_merge($probeLengthsPerNode[$currentNode], $probeLengths[$indexLength]);
                $currentNode = ($currentNode + 1) % $nodes;
            }

            for ($i = 0; $i < $nodes; $i++) {
                $probeLengthsPerNode[$i] = array_unique($probeLengthsPerNode[$i]);
                asort($indexLengthsPerNode[$i]);
                asort($probeLengthsPerNode[$i]);
            }

            //echo "== Costs per node ==\n";
            $maxSpeicherbedarf = 0;
            foreach ($costsPerNode as $node => $costs) {
                if ($minCosts > $costs) {
                    $minCosts = $costs;
                }
                if ($maxCosts < $costs) {
                    $maxCosts = $costs;
                }

                // Compute number of records to be indexed:
                $anzahlRecIx = 0;
                $speicherbedarfIx = 0;
                foreach ($indexLengthsPerNode[$node] as $l) {
                    $anzahlRecIx += $lengthCounts[$l];
                    $pl = $l - ceil($theta * $l) + 1;
                    $speicherbedarfIx += $lengthCounts[$l] * $pl;
                }
                $speicherbedarfIx *= 12; // 12 bytes/IndexEntry-struct
                $speicherbedarfIx /= 1073741824; // Gigabyte

                // Compute number records to probe:
                $anzahlProbeRecs = 0;
                $speicherbedarfProbeRecs = 0;
                
                foreach ($probeLengthsPerNode[$node] as $l) {
                    $tmp = intval($lengthCounts[$l] / $mod);
                    $anzahlProbeRecs += $tmp;
                    $speicherbedarfProbeRecs += $tmp * (60 + $l*4);
                    
                }
                $speicherbedarfProbeRecs /= 1073741824; // Gigabyte

                $candFact = 1/3; // empirical factor

                // compute candidate RAM demand: 24 due to number of threads
                $speicherbedarfCandidates = ($anzahlRecIx * 12 * 24 * $candFact) / 1073741824; // Gigabyte


                $gesamtSpeicherbedarf = $speicherbedarfIx + $speicherbedarfProbeRecs + $speicherbedarfCandidates;
                if ($gesamtSpeicherbedarf > $maxSpeicherbedarf) {
                    $maxSpeicherbedarf = $gesamtSpeicherbedarf;
                }

                if ($showDetails) {
                    echo "#$node => #indexRecs: $anzahlRecIx ramIx: $speicherbedarfIx #probeRecs: $anzahlProbeRecs ramProbe: $speicherbedarfProbeRecs ramCand: $speicherbedarfCandidates ramTotal: $gesamtSpeicherbedarf costs: $costs\n";
                }

            }
            if ($minCosts > 0) {
                $relation = $maxCosts / $minCosts;
            } else {
                $relation = PHP_INT_MAX;
            }
            if ($showDetails) {
                echo "#MinCosts ".$minCosts." maxCosts ".$maxCosts." relation $relation maxRamDemand: $maxSpeicherbedarf\n#\n";
            }

            // finde opt:
            if ((!$opt && $maxSpeicherbedarf < $maxMem) ||
                 ($opt && $maxSpeicherbedarf < $maxMem && $relation < $optRelation)) {
                $optRelation = $relation;
                $opt = ['n' => $nodes, 'm' => $mod];
            }

            $mod *= 2;
            $nodes = $totalNodes / $mod;
            if ($nodes < 4) {
                if ($opt) {
                    // we got an acceptable parameter combination
                    break;
                }
                $totalNodes *= 2;
                $mod = $minMod;
                $nodes = $totalNodes / $mod;
            }
        } // end while totalNodes
        echo $dataset.';'.$theta.';'.$opt['m'].';'.$opt['n']."\n";

    } // end foreach theta
} // end foreach dataset

