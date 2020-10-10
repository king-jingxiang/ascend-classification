# ascend-classification
使用昇腾AI弹性云服务器实现图像分类应用
实验指导用户完成基于华为昇腾弹性云服务器的图像分类应用。
1.准备环境
1.1.预置环境
注意：开始实验之前请点击手册上方“预置实验环境”按钮。

实验开始之前，为什么需要先预置实验环境？
预置实验环境约等待【1-3分钟】。环境预置会生成名称为ecs-image弹性云服务器ECS，创建配置相关的VPC、弹性公网IP、安全组，并在“ecs-image”上安装Mind Studio开发工具并配置。

1.2.登录华为云
进入【实验操作桌面】，打开火狐浏览器进入华为云，点击“登录”进入，选择【IAM用户登录】模式，输入系统为您分配的华为云实验账号名、用户名和密码登录华为云，如下图所示：

系统提供的华为云实验账号和真实的华为云账号有什么区别？
注意：账号信息详见实验手册上方，切勿使用您自己的华为云账号登录。


2.配置工程
2.1.配置连接
切换至【实验操作桌面】双击图标“Xfce 终端”打开命令行界面，输入以下命令启动“Mind Studio”。

拷贝代码
sh MindStudio-ubuntu/bin/MindStudio.sh
启动成功，保持当前命令行开启，请勿关闭。

在成功启动“Mind Studio”的界面点击“Run”-> “Edit Configuration…”如下图所示：


在弹出的页面点击“+”如下图所示：


切换至实验桌面的浏览器，鼠标移动到云桌面浏览器页面中左侧菜单栏，点击服务列表-> “弹性云服务器ECS”，找到名称为“ecs-image”服务器的弹性公网IP，切换至“Mind Studio”，在新弹出的配置页面点击“+”，填入该弹性公网IP，如下图所示：


点击“OK”，成功添加如下图所示：


点击“OK”关闭连接配置页，再点击“OK”关闭“Edit Configuration...”页面。

2.2.模型转换
在Mind Studio的菜单栏找到“Tools”->“Model Convert”打开模型转换界面，如下图所示：


添加“ /home/user/AscendProjects/”下要转换的模型“resnet18.prototxt”，选中后点击“打开”如下图所示：


打开后如下图所示：


点击“Next”，输入模型的图片宽高默认是“224，224”，如下图所示：


点击“Next”-> “Next”，开启“AIPP”，输入模型的图片格式选择“YUV420SP_U8”，图片模型格式选择“BGR888_U8”如下图所示：


点击“Finish”进行转换，模型转换成功，如下图所示：


模型转换成功后，om的离线模型存放地址为“/home/ascend/modelzoo/resnet18/device/resnet18.om”。在【实验操作桌面】重新打开一个Xfce 终端执行以下命令，将resnet18.om放到工程目录下：

拷贝代码
cp /home/user/modelzoo/resnet18/device/resnet18.om /home/user/AscendProjects/sample-classification/script/
执行完成，在Mind Studio可以看到“resnet18.om”文件，如下图所示：


3.关键代码补充
3.1. 数据获取引擎关键代码编写
数据获取引擎是指“/home/user/AscendProjects/sample-classification/src/general_image”目录下的“general_image.cpp”和“general_image.h”。
复制以下代码，在MindStudio的左侧栏找到源文件“general_image.cpp”双击打开，找到如下图所示位置的【//TODO:】后回车下一行（文件第136行），添加如下代码，打开本地图片文件。

拷贝代码
  struct dirent *dirent_ptr = nullptr;
  DIR *dir = nullptr;
  if (IsDirectory(path)) {
    dir = opendir(path.c_str());
    while ((dirent_ptr = readdir(dir)) != nullptr) {
      // skip . and ..
      if (dirent_ptr->d_name[0] == '.') {
        continue;
      }
      // file path
      string full_path = path + kPathSeparator + dirent_ptr->d_name;
      // directory need recursion
      if (IsDirectory(full_path)) {
        GetPathFiles(full_path, file_vec);
      } else {
        // put file
        file_vec.emplace_back(full_path);
      }
    }
  } else {
    file_vec.emplace_back(path);
  }
结果如下图所示：


复制以下代码，在源文件“general_image.cpp” 找到如下图所示位置的【//TODO:】后回车下一行（文件第163行），添加如下代码，通过opencv获取单张图片数据。

拷贝代码
  // read image using OPENCV
  cv::Mat mat = cv::imread(image_path, CV_LOAD_IMAGE_COLOR);
  if (mat.empty()) {
    ERROR_LOG("Failed to deal file=%s. Reason: read image failed.",
              image_path.c_str());
    return false;
  }

  // set property
  image_handle->path = image_path;
  image_handle->width = mat.cols;
  image_handle->height = mat.rows;

  // set image data
  uint32_t size = mat.total() * mat.channels();
  u_int8_t *image_buf_ptr = new (nothrow) u_int8_t[size];
  if (image_buf_ptr == nullptr) {
    HIAI_ENGINE_LOG("new image buffer failed, size=%d!", size);
    ERROR_LOG("Failed to deal file=%s. Reason: new image buffer failed.",
              image_path.c_str());
    return false;
  }

  error_t mem_ret = memcpy_s(image_buf_ptr, size, mat.ptr<u_int8_t>(),
                             mat.total() * mat.channels());
  if (mem_ret != EOK) {
    delete[] image_buf_ptr;
    ERROR_LOG("Failed to deal file=%s. Reason: memcpy_s failed.",
              image_path.c_str());
    image_buf_ptr = nullptr;
    return false;
  }

  image_handle->size = size;
  image_handle->data.reset(image_buf_ptr,
                                      default_delete<u_int8_t[]>());
  return true;
结果如下图所示：


复制以下代码，在源文件“general_image.cpp” 找到如下图所示位置的【//TODO:】后回车下一行（文件第209行），添加如下代码，通过SendData发送EngineEvbTrans数据到下一个引擎。

拷贝代码
hiai_ret = SendData(kSendDataPort, "EngineEvbTrans",
					static_pointer_cast<void>(image_handle));
结果如下图所示：


按键“Ctrl+S”保存文件。

3.2. 数据预处理及模型推理引擎
数据预处理及模型推理引擎是指”/home/user/AscendProjects/sample-classification/src/general_inference”目录下的“general_inference.cpp”和“general_inference.h”，
调用EZDVPP对输入数据进行格式转换和大小缩放。双击打开源文件“general_inference.cpp”，找到如下图所示位置的【//TODO:】后回车下一行（文件第150行），添加如下代码。

拷贝代码
resize_para.dest_resolution.width = dst_width;
resize_para.dest_resolution.height = dst_height;

调用Process接口进行模型推理。在源文件“general_inference.cpp”中找到如下图所示位置的【//TODO:】后回车下一行（文件第212行），添加如下代码。

拷贝代码
ret = ai_model_manager_->Process(ai_context, input_data_vec, output_data_vec,
							   kAiModelProcessTimeout);
// process failed, also need to send data to post process
if (ret != hiai::SUCCESS) {
	HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "call Process failed");
	return false;
}
结果如下图所示：


按键“Ctrl+S”保存文件。

3.3.数据后处理引擎
数据后处理引擎是指“/home/user/AscendProjects/sample-classification/src/ general_post”目录下的“general_post.cpp”和“general_post.h”。
双击打开源文件“general_post.cpp”， 找到如下图所示位置的【//TODO:】后回车下一行（文件第135行），添加如下代码，对推理的结果进行处理，筛选出置信度最高的物体类别。

拷贝代码
// sort by original data
sort(idx.begin(), idx.end(), [&varr](size_t i1, size_t i2) {return varr[i1] > varr[i2];});
结果如下图所示：


将分类结果通过opencv写入到本地图片。在源文件“general_post.cpp”中找到如下图所示位置的【//TODO:】后回车下一行（文件第200行），添加如下代码。

拷贝代码
save_ret = imwrite(output_file,mat);
if (!save_ret) {
  ERROR_LOG("[SaveFilePostProcess] Failed to deal file=%s. Reason: save image failed.",
		  output_path.c_str());
  return HIAI_ERROR;
}
结果如下图所示：


按键“Ctrl+S”保存文件。

4.编译并查看结果
4.1.编译项目代码
在各个引擎中添加完关键代码后，在Mind Studio的顶部菜单栏点击“Run” -> “Run ‘ sample-classification’”运行程序，程序运行成功会读取result_files中的内容，回传到本地，结果如下图所示：


双击打开图片内容如下图：
（下图out_2008_000159.jpg）


（下图out_2008_000160.jpg）


（下图out_2008_000161.jpg）


4.2.Profiling查看推理能力
工程编译完成后，在Mind Studio的顶部菜单栏点击“Run”->“Edit Configurations”打开设置页面，找到“Profiling”区域并启用，点击“OK”，如下图所示：


点击“Run” -> “Run ‘ sample-classification’”运行程序，Profiling执行成功后，在左侧栏项目名称“sample-classification”右键选择“View Profiling Result”菜单，查看Profiling数据结果，如下图所示：


在Mind Studio的底部控制台选择“Timeline”，在输出内容中找到“device”侧模型推理任务的时间耗时，如下图所示：


将鼠标停留至上图右侧红框标识区域，可以查看图片模型的推理耗时（3个区块表示3个图片模型的推理耗时）
例如下图为out_2008_000159.jpg推理耗时：


至此实验全部完成。
此外，用户可自行尝试用googlenet分类模型替换Resnet18分类模型，其中注意事项包括：
1.替换网络和权重文件。googlenet分类模型的网络和权重文件下载地址是：https://obs-model-ascend.obs.cn-east-2.myhuaweicloud.com/googlenet/googlenet.caffemodel和https://obs-model-ascend.obs.cn-east-2.myhuaweicloud.com/googlenet/googlenet.prototxt；
2.修改部署脚本。把deploy.sh部署脚本中的”resnet18”修改为“googlenet“。
其他操作步骤与Resnet18分类模型一致。

使用昇腾AI弹性云服务器实现图像分类应用
结束实验00 : 56 : 33
已完成100%


