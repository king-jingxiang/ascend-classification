#include <iostream>
#include <stdio.h>
#include "data_type.h"

/**
 * @brief: serialize for ImageInfo
 */
template<class Archive>
void serialize(Archive& ar, ImageInfo& data) {
  ar(data.path);
  ar(data.width);
  ar(data.height);
  ar(data.size);
  if (data.size > 0 && data.data.get() == nullptr) {
    data.data.reset(new u_int8_t[data.size]);
  }
  ar(cereal::binary_data(data.data.get(), data.size * sizeof(u_int8_t)));
}

/**
 * @brief: serialize for Output
 */
template<class Archive>
void serialize(Archive& ar, Output& data) {
  ar(data.size);
  if (data.size > 0 && data.data.get() == nullptr) {
    data.data.reset(new u_int8_t[data.size]);
  }
  ar(cereal::binary_data(data.data.get(), data.size * sizeof(u_int8_t)));
}


/**
 * @brief: serialize for ErrorInferenceMsg
 */
template<class Archive>
void serialize(Archive& ar, ErrorInferenceMsg& data) {
  ar(data.error, data.err_msg);
}



/**
 * @brief: serialize for EngineTrans
 */
template<class Archive>
void serialize(Archive& ar, EngineTrans& data) {
  ar(data.image_info, data.err_msg, data.inference_res,
     data.is_finished);
}
HIAI_REGISTER_DATA_TYPE("EngineTrans", EngineTrans);

/**
 * @brief: serialize for EngineEvbTrans
 */
template<class Archive>
void serialize(Archive& ar, EngineEvbTrans& data) {
  ar(data.width, data.height, data.path, data.is_finished);
}


//The new version of serialize function
void hiaiSerializeFunc(void *input_ptr, std::string& ctrl_str, uint8_t*& data_ptr, uint32_t& data_len) {
    if (input_ptr == nullptr) {
        return;
    }
    EngineEvbTrans* imageinfo = (EngineEvbTrans*)input_ptr;

    // 返回图片数据buffer与描述buffer的大小
    data_ptr = (uint8_t*)imageinfo->data.get();
    data_len = (uint32_t)imageinfo->size;

    // 数据序列化
    std::ostringstream outputStr;
    cereal::PortableBinaryOutputArchive archive(outputStr);
    archive((*imageinfo));
    ctrl_str = outputStr.str();
}


//The new version of deserialize function
std::shared_ptr<void> hiaiDeSerializeFunc(const char* ctrl_ptr, const uint32_t& ctr_len, const uint8_t* data_ptr, const uint32_t& data_len) {
    if (ctrl_ptr == nullptr) {
        return nullptr;
    }
    // 为接收序列化数据创建buffer
    std::shared_ptr<EngineEvbTrans> imageinfo = std::make_shared<EngineEvbTrans>(); 

    std::istringstream inputStream(std::string(ctrl_ptr, ctr_len));
    cereal::PortableBinaryInputArchive archive(inputStream);
    archive((*imageinfo));

    std::shared_ptr<EngineTrans> image_handle = std::make_shared<EngineTrans>(); 
    if (true == imageinfo->is_finished) {
        image_handle->is_finished = true;
        return image_handle;
    }
    
    image_handle->image_info.path = imageinfo->path;
    image_handle->image_info.width = imageinfo->width;
    image_handle->image_info.height = imageinfo->height;

    image_handle->image_info.size = data_len;
    // 指定存有图片数据的智能指针析构器
    image_handle->image_info.data.reset((uint8_t*)data_ptr, hiai::Graph::ReleaseDataBuffer);
    return std::static_pointer_cast<void>(image_handle);
}

HIAI_REGISTER_SERIALIZE_FUNC("EngineEvbTrans", EngineEvbTrans, hiaiSerializeFunc, hiaiDeSerializeFunc);

/**
* @brief: create folder to store the detection results
* the folder name on the host will be "result_files/enginename"
*/
HIAI_StatusT CreateFolder(std::string folderPath, mode_t mode) {
    int folder_exist = access(folderPath.c_str(), W_OK);
    if (-1 == folder_exist) {
        if (mkdir(folderPath.c_str(), mode) == -1) {
            HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "Failed to create folder %s.", folderPath.c_str());
            return HIAI_ERROR;
        }
    }
    return HIAI_OK;
}

