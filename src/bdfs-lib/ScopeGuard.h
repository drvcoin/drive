// Copyright (c) 2018 Drive Foundation

#pragma once

#include <vector>
#include <functional>

namespace bdfs
{
  class ScopeGuard
  {
  public:

    using ExitCallback = std::function<void()>;

  public:

    ScopeGuard() = default;

    explicit ScopeGuard(ExitCallback callback);

    ~ScopeGuard();

    void Register(ExitCallback callback);

    void Reset();

  private:

    std::vector<ExitCallback> callbacks;
  };
}
