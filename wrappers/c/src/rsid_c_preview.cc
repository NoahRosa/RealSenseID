// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include "rsid_c/rsid_preview.h"
#include <memory>

namespace
{

RealSenseID::Image c_img_to_api_image(const rsid_image* in_img)
{
    RealSenseID::Image out_img;
    out_img.buffer = in_img->buffer;
    out_img.size = in_img->size;
    out_img.width = in_img->width;
    out_img.height = in_img->height;
    out_img.stride = in_img->stride;
    out_img.number = in_img->number;
    out_img.metadata.led = in_img->metadata.led;
    out_img.metadata.projector = in_img->metadata.projector;
    out_img.metadata.sensor_id = in_img->metadata.sensor_id;
    out_img.metadata.status = in_img->metadata.status;
    out_img.metadata.timestamp = in_img->metadata.timestamp;
    return out_img;
}

rsid_image api_image_to_c_img(const RealSenseID::Image* in_img)
{
    rsid_image out_img;
    out_img.buffer = in_img->buffer;
    out_img.size = in_img->size;
    out_img.width = in_img->width;
    out_img.height = in_img->height;
    out_img.stride = in_img->stride;
    out_img.number = in_img->number;
    out_img.metadata.led = in_img->metadata.led;
    out_img.metadata.projector = in_img->metadata.projector;
    out_img.metadata.sensor_id = in_img->metadata.sensor_id;
    out_img.metadata.status = in_img->metadata.status;
    out_img.metadata.timestamp = in_img->metadata.timestamp;
    return out_img;
}

class PreviewClbk : public RealSenseID::PreviewImageReadyCallback
{
public:
    explicit PreviewClbk(rsid_preview_clbk c_clbk, void* ctx) : m_callback {c_clbk}, m_ctx {ctx}
    {
    }

    void OnPreviewImageReady(const RealSenseID::Image image) override
    {
        if (m_callback)
        {
            m_callback(api_image_to_c_img(&image), m_ctx);
        }
    }

private:
    rsid_preview_clbk m_callback;
    void* m_ctx;
};
} // namespace

static std::unique_ptr<PreviewClbk> s_preview_clbk;

rsid_preview* rsid_create_preview(const rsid_preview_config* preview_config)
{
    RealSenseID::PreviewConfig config;
    config.cameraNumber = preview_config->camera_number;
    config.previewMode = static_cast<RealSenseID::PreviewMode>(preview_config->preview_mode);
    auto* preview_impl = new RealSenseID::Preview(config);

    if (preview_impl == nullptr)
    {
        return nullptr;
    }

    auto* rv = new rsid_preview();
    rv->_impl = preview_impl;
    return rv;
}

void rsid_destroy_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        delete preview_impl;
        s_preview_clbk.reset();
    }
    catch (...)
    {
    }
    delete preview_handle;
}

int rsid_start_preview(rsid_preview* preview_handle, rsid_preview_clbk clbk, void* ctx)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        s_preview_clbk = std::make_unique<PreviewClbk>(clbk, ctx);
        bool ok = preview_impl->StartPreview(*s_preview_clbk);
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}


int rsid_pause_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        bool ok = preview_impl->PausePreview();
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}

int rsid_resume_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        bool ok = preview_impl->ResumePreview();
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}

int rsid_stop_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        bool ok = preview_impl->StopPreview();
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}

int rsid_raw_to_rgb(rsid_preview* preview_handle,const rsid_image* in_c_img, rsid_image* out_c_img)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        RealSenseID::Image out_image = c_img_to_api_image(out_c_img);
        bool ok = preview_impl->RawToRgb(c_img_to_api_image(in_c_img), out_image);
        *out_c_img = api_image_to_c_img(&out_image);
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}
