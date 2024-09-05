#!/bin/bash

what="${1}"

#all_dates=('2024-07-25')
#all_mppcs=('1-2-3' '4-5-6' '7-8-9' '10-1-2')

#all_dates=('2024-07-23')
#all_mppcs=('3-6-8')

all_dates=('2024-07-25')
all_mppcs=('1-2-3')

### Different configurations for differente analysis
# maxPMT vs PMT V  and maxMPPC vs LED V
#max_dates=('2024-07-25')
#max_mppcs=('1-2-3' '4-5-6' '7-8-9' '10-1-2')
max_dates=('2024-07-25')
max_mppcs=('1-2-3')
# repetability 
rep_dates=('2024-07-25')
rep_mppcs=('1-2-3')
# position dependency by cycling
pos_dates=('2024-07-25')
pos_mppcs=('1-2-3')
# Quality Control
qc_dates=('2024-07-25')
qc_mppcs=('1-2-3' '4-5-6' '7-8-9' '10-1-2')
dates_file="QC_dates.txt"
mppcs_file="QC_mppcs.txt"

### For now the IV curve is totally independent

base_dir="$HOME/Documents/PhD/mppc-qc/"

pre_analyse_input_dir="${base_dir}data/"
pre_analyse_output_dir="${base_dir}pre-analyse/"
analyse_input_dir=${pre_analyse_output_dir}
analyse_output_dir="${base_dir}plots/analyse/"

if [ ${what} == "batch-pre-analyse" ]; then
	files=$(ls ${pre_analyse_input_dir}*/*.dat)
	./analysis ${what} ${pre_analyse_output_dir} ${files}

elif [ ${what} == "analyse" ]; then
	read -p  "Which analysis do you want to perform: 'maxPMT-maxMPPC', 'reproducibility', 'position_dependency' or 'QC'? " analysis_type
		if [ ${analysis_type} == "maxPMT-maxMPPC" ]; then
			for date in "${max_dates[@]}"
			do
				for mppcs in "${max_mppcs[@]}"
				do
					./analysis ${what} ${analysis_type} ${date} ${mppcs} ${analyse_input_dir} ${analyse_output_dir}
				done
			done
		elif [ ${analysis_type} == "reproducibility" ];then 
			echo "Sorry this is not supported yet..." ### To be removed onece done
			break
			for date in "${rep_dates[@]}"
			do
				for mppcs in "${rep_mppcs[@]}"
				do
					./analysis ${what} ${analysis_type} ${date} ${mppcs} ${analyse_input_dir} ${analyse_output_dir}
				done
			done
		elif [ ${analysis_type} == "position_dependency" ];then
			echo "Sorry this is not supported yet..." ### To be removed onece done
			break
			for date in "${pos_dates[@]}"
			do
				for mppcs in "${pos_mppcs[@]}"
				do
					./analysis ${what} ${analysis_type} ${date} ${mppcs} ${analyse_input_dir} ${analyse_output_dir}
				done
			done
		elif [ ${analysis_type} == "QC" ];then
			echo "Sorry this is not supported yet..." ### To be removed onece done
			break
			if [! -f ${dates_file}];then
				rm  -rf ${dates_file}
			fi
			for date in "${qc_dates[@]}"
			do
				${date} >> ${dates_file}
			done
			if [! -f ${mppcs_file}];then
				rm  -rf ${mppcs_file}
			fi
			for mppcs in "${qc_mppcs[@]}"
			do
				${mppcs} >> ${mppcs_file}
			done
			./analysis ${what} ${analysis_type} ${dates_file} ${mppcs_file} ${analyse_input_dir} ${analyse_output_dir}
		else
			echo "ERROR: please choose among the given possibilities..."
		fi
else
	echo "ERROR: choose between 'pre-analyse' and 'analyse'..."
fi
