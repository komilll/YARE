#pragma once

#ifndef _RAYTRACING_SHADERS_HELPER_H_
#define _RAYTRACING_SHADERS_HELPER_H_

#include "pch.h"
#include "dxc/dxcapi.h"
#include "dxc/dxcapi.use.h"

struct D3D12ShaderCompilerInfo
{
	dxc::DxcDllSupport		DxcDllHelper;
	IDxcCompiler* compiler = nullptr;
	IDxcLibrary* library = nullptr;
};

struct D3D12ShaderInfo
{
	LPCWSTR		filename = nullptr;
	LPCWSTR		entryPoint = nullptr;
	LPCWSTR		targetProfile = nullptr;
	LPCWSTR* arguments = nullptr;
	DxcDefine* defines = nullptr;
	UINT32		argCount = 0;
	UINT32		defineCount = 0;

	D3D12ShaderInfo() {}
	D3D12ShaderInfo(LPCWSTR inFilename, LPCWSTR inEntryPoint, LPCWSTR inProfile)
	{
		filename = inFilename;
		entryPoint = inEntryPoint;
		targetProfile = inProfile;
	}
};

struct RtProgram
{
	D3D12ShaderInfo			info = {};
	IDxcBlob* blob = nullptr;
	ID3D12RootSignature* pRootSignature = nullptr;

	D3D12_DXIL_LIBRARY_DESC	dxilLibDesc{};
	D3D12_EXPORT_DESC		exportDesc{};
	D3D12_STATE_SUBOBJECT	subobject{};
	std::wstring			exportName{};
	LPCWSTR					name{};
	LPCWSTR					exportToRename{};

	RtProgram()
	{
		exportDesc.ExportToRename = nullptr;
	}

	RtProgram(D3D12ShaderInfo shaderInfo)
	{
		info = shaderInfo;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		exportName = shaderInfo.entryPoint;
		exportDesc.ExportToRename = nullptr;
		exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
	}

	void SetBytecode()
	{
		exportDesc.Name = exportName.c_str();

		dxilLibDesc.NumExports = 1;
		dxilLibDesc.pExports = &exportDesc;
		dxilLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
		dxilLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();

		subobject.pDesc = &dxilLibDesc;
	}

};

struct HitProgram
{
	RtProgram ahs;
	RtProgram chs;

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc = {};
	D3D12_STATE_SUBOBJECT subobject = {};
	LPCWSTR					name{};
	LPCWSTR					exportToRename{};

	HitProgram() {}
	HitProgram(LPCWSTR name) : exportName(name)
	{
		desc = {};
		desc.HitGroupExport = exportName.c_str();
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subobject.pDesc = &desc;
	}

	void SetExports(bool anyHit)
	{
		desc.HitGroupExport = exportName.c_str();
		if (anyHit) desc.AnyHitShaderImport = ahs.exportDesc.Name;
		desc.ClosestHitShaderImport = chs.exportDesc.Name;
	}

};

struct HitGroup
{
	RtProgram rayGenShader;
	RtProgram missShader;
	HitProgram hitShader;

	LPCWSTR hitGroupName;
};

#endif // !_RAYTRACING_SHADERS_HELPER_H_
