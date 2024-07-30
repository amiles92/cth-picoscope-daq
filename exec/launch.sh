#!/bin/bash

what="${1}"

### Different configurations for differente analysis
# maxPMT vs PMT V  and maxMPPC vs LED V
max_dates=('2024-07-23')
max_mppcs=('3-6-8')
# repetability 
rep_dates=('2024-07-23')
rep_mppcs=('1-2-3')
# position dependency by cycling
pos_dates=('2024-07-23')
pos_mppcs=('1-2-3')
# Quality Control
qc_dates=('2024-07-23')
qc_mppcs=('1-2-3' '4-5-6' '7-8-9' '10-1-2')
dates_file="QC_dates.txt"
mppcs_file="QC_mppcs.txt"

### For now the IV curve is totally independent

pre_analyse_input_dir='/home/chadeau/Documents/codes/MPPC_QC/data-files/'
pre_analyse_output_dir='/home/chadeau/Documents/codes/MPPC_QC/cth-picoscope-daq/'
analyse_input_dir=${pre_analyse_output_dir}
analyse_output_dir=${pre_analyse_output_dir}

if [ ${what} == "pre-analyse" ]; then
	for date in "${all_dates[@]}"
	do
		cd ../plots/pre-analyse/
		if [ ! -d "${date}" ];then 
			mkdir ${date}
		fi
		cd ../../exec/
		for mppcs in "${all_mppcs[@]}"
		do
			./analysis ${what} ${date} ${mppcs} ${pre_analyse_input_dir} ${pre_analyse_output_dir}
		done
	done
elif [ ${what} == "analyse" ]; then
	read -p  "Which analysis do you want to perform: 'maxPMT-maxMPPC', 'repetability', 'position_dependency' or 'QC'? " analysis_type
		if [ ${analysis_type} == "maxPMT-maxMPPC" ]; then
			for date in "${all_dates[@]}"
			do
				for mppcs in "${all_mppcs[@]}"
				do
					./analysis ${what} ${analysis_type} ${date} ${mppcs} ${input} ${output}
				done
			done
		elif [ ${analysis_type} == "repetability" ];then
			for date in "${all_dates[@]}"
			do
				for mppcs in "${all_mppcs[@]}"
				do
					./analysis ${what} ${analysis_type} ${date} ${mppcs} ${input} ${output}
				done
			done
		elif [ ${analysis_type} == "position_dependency" ];then
			for date in "${all_dates[@]}"
			do
				for mppcs in "${all_mppcs[@]}"
				do
					./analysis ${what} ${analysis_type} ${date} ${mppcs} ${input} ${output}
				done
			done
		elif [ ${analysis_type} == "QC" ];then
			if [! -f ${dates_file}];then
				rm  -rf ${dates_file}
			fi
			for date in "${all_dates[@]}"
			do
				${date} >> ${dates_file}
			done
			if [! -f ${mppcs_file}];then
				rm  -rf ${mppcs_file}
			fi
			for mppcs in "${all_mppcs[@]}"
			do
				${mppcs} >> ${mppcs_file}
			done
			./analysis ${what} ${analysis_type} ${dates_file} ${mppcs_file} ${input} ${output}
		else
			echo "ERROR: please choose among the given possibilities..."
		fi
else
	echo "ERROR: choose between 'pre-analyse' and 'analyse'..."
fi
