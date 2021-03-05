#pragma once
#ifndef _DEPTH_STENCIL_MANAGER_H
#define _DEPTH_STENCIL_MANAGER_H

#include "pch.h"

class DepthStencilManager
{
public:
	static CD3DX12_DEPTH_STENCIL_DESC1 CreateDefaultDepthStencilDesc();
};

#endif //_DEPTH_STENCIL_MANAGER_H