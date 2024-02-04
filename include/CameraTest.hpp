#ifndef ENERGYLEAF_V1_SOURCE_CAMERATEST
#define ENERGYLEAF_V1_SOURCE_CAMERATEST

#include <Extras/Vision/Camera/AbstractCamera.hpp>

class CameraTest : public Energyleaf::Stream::V1::Extras::Vision::AbstractCamera<int> {
public:
    using CameraConfig = int;

    CameraTest() : AbstractCamera() {
        p = 3;
    }

    virtual ~CameraTest() {
        //internalStop();
    };

    virtual const int& getConfig() const {
       return p;
    }

    virtual void setConfig(int&& config) {
    }

    virtual void setConfig(int& config) {
    }
private:
    int p;
protected:
    void internalStart() override{
    }
    void internalStop() override{
    }
    [[nodiscard]] Energyleaf::Stream::V1::Types::Image getInternalImage() const override {
        return {};
    }
};

#endif