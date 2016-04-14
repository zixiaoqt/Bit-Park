#ifndef _SCOPEGUARD_H_
#define _SCOPEGUARD_H_
#include <functional>
#include <new>

namespace parkingserver {

class ScopeGuardImplBase {
  public:
    void dismiss() {
        dismissed_ = true;
    }

  protected:
    ScopeGuardImplBase()
            : dismissed_(false) {}

    ScopeGuardImplBase(ScopeGuardImplBase&& other)
    : dismissed_(other.dismissed_) {
        other.dismissed_ = true;
    }

    bool dismissed_;
};

template <typename FunctionType>
class ScopeGuardImpl : public ScopeGuardImplBase {
  public:
    explicit ScopeGuardImpl(const FunctionType& fn)
            : function_(fn) {}

    explicit ScopeGuardImpl(FunctionType&& fn)
            : function_(std::move(fn)) {}

    ScopeGuardImpl(ScopeGuardImpl&& other)
            : ScopeGuardImplBase(std::move(other))
            , function_(std::move(other.function_)) {
    }

    ~ScopeGuardImpl() {
        if (!dismissed_) {
            execute();
        }
    }

  private:
    void* operator new(size_t) = delete;

    void execute() { function_(); }

    FunctionType function_;
};

template <typename FunctionType>
ScopeGuardImpl<typename std::decay<FunctionType>::type>
makeGuard(FunctionType&& fn) {
    return ScopeGuardImpl<typename std::decay<FunctionType>::type>(
        std::forward<FunctionType>(fn));
}

}
#endif
