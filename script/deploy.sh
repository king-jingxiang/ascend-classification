#!/bin/bash
script_path="$( cd "$(dirname "$0")" ; pwd -P )"
app_path="${script_path}/../src"
remote_port="22118"

function build_common()
{
    echo "build common lib..."
    if [ ! -d "${HOME}/ascend_ddk" ];then
        mkdir $HOME/ascend_ddk
        if [[ $? -ne 0 ]];then
            echo "ERROR: Execute mkdir command failed, Please check your environment"
            return 1
        fi
    fi
    bash ${script_path}/build_ezdvpp.sh ${remote_host}
    if [ $? -ne 0 ];then
        echo "ERROR: Failed to deploy ezdvpp"
        return 1
    fi
    return 0
}


function main()
{    
    build_common
    if [ $? -ne 0 ];then
        echo "ERROR: Failed to deploy common lib"
        return 1
    fi

    rm -rf /home/HwHiAiUser/HIAI_DATANDMODELSET/workspace_mind_studio/resource/
    if [ $? -ne 0 ];then
        echo "ERROR: Failed to rm ~/HIAI_DATANDMODELSET/workspace_mind_studio/resource/"
        return 1
    fi


	echo "Modify param information in graph.config..."
	for om_name in $(find ${script_path}/ -name "*.om");do
        om_name=$(basename ${om_name})
        if [ "resnet18" != ${om_name%.*} ];then
            continue
        fi

        cp ${script_path}/graph.template ${app_path}/graph.config
        sed -i "s#\${MODEL_PATH}#../../script/${om_name}#g"  ${app_path}/graph.config
        if [ $? != 0 ];then
            echo "gengrate graph.config error !"
            return 1
        fi 
        return 0
    done

    echo "please push model file in sample-classification/script "
    return 1
}
main

