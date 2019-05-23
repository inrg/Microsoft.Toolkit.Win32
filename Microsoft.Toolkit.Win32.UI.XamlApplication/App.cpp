﻿#include "pch.h"

#include "App.h"

namespace xaml = ::winrt::Windows::UI::Xaml;

extern "C" {
    WINBASEAPI HMODULE WINAPI LoadLibraryExW(_In_ LPCWSTR lpLibFileName, _Reserved_ HANDLE hFile, _In_ DWORD dwFlags);
    WINBASEAPI HMODULE WINAPI GetModuleHandleW(_In_opt_ LPCWSTR lpModuleName);
    WINUSERAPI BOOL WINAPI PeekMessageW(_Out_ LPMSG lpMsg, _In_opt_ HWND hWnd, _In_ UINT wMsgFilterMin, _In_ UINT wMsgFilterMax, _In_ UINT wRemoveMsg);
    WINUSERAPI LRESULT WINAPI DispatchMessageW(_In_ CONST MSG* lpMsg);
}

namespace winrt::Microsoft::Toolkit::Win32::UI::XamlApplication::implementation
{
    App::App(winrt::Windows::Foundation::Collections::IVector<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider> providers)
    {
        for(auto provider : providers)
        {
            m_providers.Append(provider);
        }

        Initialize();
    }

    App::App()
    {
    }

    void App::Initialize()
    {
        const auto out = outer();
        if (out)
        {
            winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider provider(nullptr);
            winrt::check_hresult(out->QueryInterface(
                winrt::guid_of<winrt::Windows::UI::Xaml::Markup::IXamlMetadataProvider>(),
                reinterpret_cast<void**>(winrt::put_abi(provider))));
            m_providers.Append(provider);
        }

        const auto dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
        if (!dispatcherQueue)
        {
            m_executionMode = ExecutionMode::Win32;
            m_windowsXamlManager = xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
        }
        else
        {
            m_executionMode = ExecutionMode::UWP;
        }
    }

    winrt::Windows::Foundation::IClosable App::WindowsXamlManager() const
    {
        return m_windowsXamlManager;
    }

    void App::Close()
    {
        if (m_bIsClosed)
        {
            return;
        }

        m_bIsClosed = true;

        m_windowsXamlManager.Close();
        m_providers.Clear();
        m_windowsXamlManager = nullptr;

        Exit();
        {
            MSG msg = {};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::DispatchMessageW(&msg);
            }
        }
    }

    App::~App()
    {
        Close();
    }

    xaml::Markup::IXamlType App::GetXamlType(xaml::Interop::TypeName const& type)
    {
        for (const auto& provider : m_providers)
        {
            const auto xamlType = provider.GetXamlType(type);
            if (xamlType != nullptr)
            {
                return xamlType;
            }
        }

        return nullptr;
    }

    xaml::Markup::IXamlType App::GetXamlType(winrt::hstring const& fullName)
    {
        for (const auto& provider : m_providers)
        {
            const auto xamlType = provider.GetXamlType(fullName);
            if (xamlType != nullptr)
            {
                return xamlType;
            }
        }

        return nullptr;
    }

    winrt::com_array<xaml::Markup::XmlnsDefinition> App::GetXmlnsDefinitions()
    {
        std::list<xaml::Markup::XmlnsDefinition> definitions;
        for (const auto& provider : m_providers)
        {
            auto defs = provider.GetXmlnsDefinitions();
            for (const auto& def : defs)
            {
                definitions.insert(definitions.begin(), def);
            }
        }

        return winrt::com_array<xaml::Markup::XmlnsDefinition>(definitions.begin(), definitions.end());
    }

    winrt::Windows::Foundation::Collections::IVector<xaml::Markup::IXamlMetadataProvider> App::MetadataProviders()
    {
        return m_providers;
    }
}

namespace winrt::Microsoft::Toolkit::Win32::UI::XamlApplication::factory_implementation
{
    App::App()
    {
        // Workaround a bug where twinapi.appcore.dll and threadpoolwinrt.dll gets loaded after it has been unloaded
        // because of a call to GetActivationFactory
        const wchar_t* preloadDlls[] = {
            L"twinapi.appcore.dll",
            L"threadpoolwinrt.dll",
        };
        for (size_t i = 0; i < m_preloadInstances.size(); i++)
        {
            const auto module = ::LoadLibraryExW(preloadDlls[i], nullptr, 0);
            m_preloadInstances[i] = module;
        }
    }

    App::~App()
    {
        for (auto module : m_preloadInstances)
        {
            ::FreeLibrary(module);
        }
    }
}