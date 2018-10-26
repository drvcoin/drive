// Copyright (c) 2018 Drive Foundation

#include "ScopeGuard.h"

namespace bdfs
{
  ScopeGuard::ScopeGuard(ScopeGuard::ExitCallback callback)
  {
    this->callbacks.emplace_back(std::move(callback));
  }


  ScopeGuard::~ScopeGuard()
  {
    for (const auto & callback : this->callbacks)
    {
      callback();
    }

    this->callbacks.clear();
  }


  void ScopeGuard::Register(ScopeGuard::ExitCallback callback)
  {
    this->callbacks.emplace_back(std::move(callback));
  }


  void ScopeGuard::Reset()
  {
    this->callbacks.clear();
  }
}
