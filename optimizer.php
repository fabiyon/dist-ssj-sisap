<?php

$datapath = "./data/"; // where data and statistics are located (only statistics used in this example)
$codepath = "./code/"; // where the mssj code resides (commented out for this example)
//$respath = "./results/"; // where the runtime results are to be written (commented out for this example)

$datasetArr = ["BPOS-10"];
$nodeArr = [4]; // parameter n
$thetaArr = [0.9, 0.75, 0.6];
$modArr = [2]; // parameter m

// write experimental results:
//$resFile = $respath.'results.csv';
//if (!file_exists($resFile)) {
//    $fp = fopen($resFile, 'w');
//    fputcsv($fp, ['dataset', 'theta','numberNodes', 'node', 'costs', 'runtime', 'candidates', 'results', 'mod', 'modgroup']);
//} else {
//    $fp = fopen($resFile, 'a');
//}

foreach ($modArr as $mod) {

foreach ($datasetArr as $dataset) {
    foreach ($nodeArr as $nodes) {
        foreach ($thetaArr as $theta) {
            for ($modgroup = 0; $modgroup < $mod; $modgroup++) {
            echo "==$dataset n=$nodes m=$mod modgroup=$modgroup theta=$theta==\n";
            $maxLength = 0;
            $lengthCounts = [];

            $h = fopen($datapath.$dataset.'-stat', 'r');
            while ($data = fgetcsv($h)) {
                $lengthCounts[$data[0]] = $data[1]; // length => count
                if ($maxLength < $data[0]) {
                    $maxLength = $data[0];
                }
            }
            fclose($h);


            // echo "== Length statistics ==\n";
            ksort($lengthCounts);
            // foreach ($lengthCounts as $lc => $cnt) {
            //     echo $lc.' => '.$cnt."\n";
            // }
            // echo "\n\n";

            // Compute probe lengths for each index length (except maxLength):
            $probeLengths = [];
            foreach ($lengthCounts as $indexLength => $count) {
                $maxProbeLength = min(floor($indexLength / $theta), $maxLength);
                for ($probeLength = $indexLength; $probeLength <= $maxProbeLength; $probeLength++) {
                    if (array_key_exists($probeLength, $lengthCounts)) {
                        $probeLengths[$indexLength][] = $probeLength; 
                    }
                }
            }

            // foreach ($probeLengths as $l => $pla) {
            //     echo $l.': '.implode(', ', $pla)."\n";
            // }
            // die();

            $sliceCosts = [];
            foreach ($probeLengths as $indexLength => $probeLengthArr) {
                $costs = 0;
                foreach ($probeLengthArr as $probeLength) {
                    $prefixLength = $probeLength - ceil($theta * $probeLength) + 1;
                    $costs += $prefixLength * $lengthCounts[$probeLength]; 
                }
                $prefixLength = $indexLength - ceil($theta * $indexLength) + 1;
                $sliceCosts[$indexLength] = $prefixLength * $lengthCounts[$indexLength] * $costs;
            }

            // echo "== Costs per index slice ==\n";
            ksort($sliceCosts);
            // foreach ($sliceCosts as $indexLength => $costs) {
            //     echo $indexLength.' => '.$costs.' '.implode(',', $probeLengths[$indexLength])."\n";
            // }
            // echo "\n\n";
            // die();


            // order by costs for greedy assignment:
            arsort($sliceCosts);
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
                $costsPerNode[$currentNode] += $costs;
                $indexLengthsPerNode[$currentNode][] = $indexLength;
                $probeLengthsPerNode[$currentNode] = array_merge($probeLengthsPerNode[$currentNode], $probeLengths[$indexLength]);
                $currentNode = ($currentNode + 1) % $nodes;
            }

            for ($i = 0; $i < $nodes; $i++) {
                $probeLengthsPerNode[$i] = array_unique($probeLengthsPerNode[$i]);
                asort($indexLengthsPerNode[$i]);
                asort($probeLengthsPerNode[$i]);
            }

            // foreach ($probeLengthsPerNode as $indexLength => $plpns) {
            //     echo $indexLength.': '.implode(', ', $plpns)."\n";
            // }
            // die();

            // echo "== Costs per node ==\n";
            foreach ($costsPerNode as $node => $costs) {
                echo "$node => $costs\n";// index lengths ".implode(',', $indexLengthsPerNode[$node])." probe lengths ".implode(',', $probeLengthsPerNode[$node])."\n";

                // Optimal parameters from multicore study: -method allph -threads 24 -pos_filter 1
                $modParam = '';
                if ($mod) {
                  $modParam = " -mod $mod -modgroup $modgroup";
                }
                $cmd = $codepath.'mssj -method allph -threads 24 -pos_filter 1 -threshold '.$theta.' -input_file '.$datapath.$dataset.' -indexLengths '.implode(',', $indexLengthsPerNode[$node]).' -probeLengths '.implode(',', $probeLengthsPerNode[$node]).$modParam;
                // echo $cmd."\n\n";
                //unset($output);
                //exec($cmd, $output);
                //print_r($output);
                //$totalDuration = 0;
                //$candidates = 0;
                //$results = 0;
                //foreach ($output as $tmp) {
                //    if (substr_count($tmp, 'Total Duration.............:') == 1) {
                //        $totalDuration = str_replace('	Total Duration.............: ', '', $tmp);
                //    }
                //    if (substr_count($tmp, '# Candidates...............:') == 1) {
                //        $candidates = str_replace('	# Candidates...............: ', '', $tmp);
                //    }
                //    if (substr_count($tmp, '# Results..................:') == 1) {
                //        $results = str_replace('	# Results..................: ', '', $tmp);
                //    }
                //}
                echo $cmd."\n\n";
                //fputcsv($fp, [$dataset, $theta, $nodes, $node, $costs, $totalDuration, $candidates, $results, $mod, $modgroup]);
                } // end for each modgroup
            }
        } // end foreach theta
    } // end foreach node
} // end foreach dataset

} // end foreach mod

//fclose($fp);
