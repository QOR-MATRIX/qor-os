// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "gpupreference.h"

#include <QtCore/QtEnvironmentVariables>

#if defined(Q_OS_WIN)
#include <dxgi1_6.h>
#include <wrl/client.h>
#endif

using namespace Qt::StringLiterals;

namespace qor::host {

QString applyGpuPreference(const QString &preference)
{
    if (qEnvironmentVariableIsSet("QT_D3D_ADAPTER_INDEX"))
        return u"QT_D3D_ADAPTER_INDEX set by environment"_s;
    if (preference == u"default"_s)
        return u"system default adapter"_s;

#if defined(Q_OS_WIN)
    using Microsoft::WRL::ComPtr;
    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return u"DXGI 1.6 unavailable; using default adapter"_s;

    const DXGI_GPU_PREFERENCE gpuPreference =
        preference == u"low-power"_s ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    ComPtr<IDXGIAdapter1> preferred;
    if (FAILED(factory->EnumAdapterByGpuPreference(0, gpuPreference, IID_PPV_ARGS(&preferred))))
        return u"no adapter for preference; using default adapter"_s;
    DXGI_ADAPTER_DESC1 wanted {};
    preferred->GetDesc1(&wanted);

    // Qt enumerates with EnumAdapters1, so translate the LUID into that index.
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index) {
        DXGI_ADAPTER_DESC1 desc {};
        adapter->GetDesc1(&desc);
        if (desc.AdapterLuid.LowPart == wanted.AdapterLuid.LowPart
            && desc.AdapterLuid.HighPart == wanted.AdapterLuid.HighPart) {
            qputenv("QT_D3D_ADAPTER_INDEX", QByteArray::number(index));
            return QString::fromWCharArray(desc.Description) + u" (adapter "_s + QString::number(index) + u')';
        }
    }
    return u"preferred adapter not enumerated; using default adapter"_s;
#else
    // Linux and macOS select GPUs through the platform (PRIME offload,
    // DRI_PRIME, or the system compositor); nothing to do here.
    return u"platform-managed adapter selection"_s;
#endif
}

} // namespace qor::host
