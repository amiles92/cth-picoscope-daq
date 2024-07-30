#!/bin/bash

### Please change the account
account=nichadea
input_file="../../../../../../../media/chadeau/MPPC-QC/QC-data/"
output_file="/group/had/muon/MPPC-QC/"

function scpFromKEKCC() {
command scp -rp -o "ProxyCommand ssh ${account}@sshcc1.kek.jp -W %h:%p" ${account}@login.cc.kek.jp:"$1" $2
}

function scpToKEKCC() {
command scp -rp -o "ProxyCommand ssh ${account}@sshcc1.kek.jp -W %h:%p" "${1}" ${account}@login.cc.kek.jp:"${2}"
}

scpToKEKCC ${input_file} ${output_file}
