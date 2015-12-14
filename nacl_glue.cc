
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/var_array_buffer.h"

#include "stitching.h"

class NaClGlueInstance : public pp::Instance, public MessageDispatcher {
 public:
  explicit NaClGlueInstance(PP_Instance instance) :
      pp::Instance(instance),
      stitching_() {
    stitching_.InitializeOpenCV(320, 240);
    stitching_.SetMessageHandler(this);
  }
  virtual ~NaClGlueInstance() {}

  virtual void HandleMessage(const pp::Var& var_message) {
    pp::VarDictionary dict(var_message);
    std::string message = dict.Get("message").AsString();
    if (message == "images") {
      int width  = dict.Get("width").AsInt();
      int height = dict.Get("height").AsInt();

      auto data1 = pp::VarArrayBuffer(dict.Get("data1"));
      auto data2 = pp::VarArrayBuffer(dict.Get("data2"));
      uint8_t* byteData1 = static_cast<uint8_t*>(data1.Map());
      uint8_t* byteData2 = static_cast<uint8_t*>(data2.Map());
      auto img1 = cv::Mat(height, width, CV_8UC4, byteData1);
      auto img2 = cv::Mat(height, width, CV_8UC4, byteData2);

      stitching_.CalculateDepthMap(img1, img2);
    }

    return;
    if (var_message.is_string()) {
      SendMessage("Command: " + var_message.AsString());
      //bool result = stitching_.CalculateHomography();
      //stitching_.CalculateDepthMap();
      //SendMessage(result ? "Done, OK" : (" - " + stitching_.last_error()));

    } else if (var_message.is_dictionary()) {
      SendMessage("I got a dictionary with image and its index");
      pp::VarDictionary dictionary(var_message);
      std::string message = dictionary.Get("message").AsString();
      if (message == "data") {
        //   int width = dictionary.Get("width").AsInt();
        //   int height = dictionary.Get("height").AsInt();
        //   int index = dictionary.Get("index").AsInt();
        //   pp::VarArrayBuffer array_buffer(dictionary.Get("data"));
        //   if (index >= 0 && index < 2 && width > 0 && height > 0) {
        //     unsigned char* pixels =
        //         static_cast<unsigned char*>(array_buffer.Map());
        //     stitching_.SetImageData(index, height, width, pixels);
        //     array_buffer.Unmap();
        //   }


        // alternately...
        int width = dictionary.Get("width").AsInt();
        int height = dictionary.Get("height").AsInt();
        auto data = pp::VarArrayBuffer(dictionary.Get("data"));
        uint8_t* byteData = static_cast<uint8_t*>(data.Map());
        auto img = cv::Mat(height, width, CV_8UC4, byteData );

        // modify message to have both images
        //stitching_.CalculateDepthMap(img);
       }
    } else {
      SendMessage("I got some message from JS I don't understand.");
    }
  }

  // MessageDispatcher interface method.
  virtual void SendMessage(std::string message) {
    PostMessage(pp::Var(message));
  }

  // MessageDispatcher interface method.
  virtual void SendMessage(pp::VarDictionary dictionary) {
    //PostMessage(pp::Var(dictionary.pp_var()));
    PostMessage(dictionary);
  }

  // MessageDispatcher interface method.
  // virtual void SendMessage(pp::VarDictionary dictionary) {
  //   PostMessage(pp::Var(dictionary.pp_var()));
  // }

 private:
  Stitching stitching_;
};

class NaClGlueModule : public pp::Module {
 public:
  NaClGlueModule() : pp::Module() {}
  virtual ~NaClGlueModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new NaClGlueInstance(instance);
  }
};

namespace pp {
Module* CreateModule() {
  return new NaClGlueModule();
}
}  // namespace pp
