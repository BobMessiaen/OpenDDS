/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_EXTERNAL_H
#define OPENDDS_DCPS_XTYPES_EXTERNAL_H

#include <dds/DCPS/unique_ptr.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

/// Simplified version of the XTypes 1.3 mapping External Members (Modern C++)
/// See section 7.5.1.2.3.2.  Differences from spec:
/// - this version uses unique ownership instead of shared ownership
/// - default constructed External<T> has a default constructed T, not empty
/// - shared_ptr constructor, get_shared_ptr, is_locked, lock are not provided
/// - External<T> can be constructed from (or assigned from) a T which is copied
/// If/when this class is generated by opendds_idl for @external, closer
/// alignment to the spec may be desired.  However, the simplified version is
/// useful for the common use case of @external (tree-like structures).
template <typename T>
class External {
public:
  External();
  explicit External(T* p, bool locked = false);
  //  External(shared_ptr<T> p);
  External(const External& other);
  // use compiler-generated destructor ~External();
  External& operator=(const External& other);

  // Additions to XTypes spec:
  External(const T& value);
  External& operator=(const T& value);

  T& operator*() { return *ptr_; }
  const T& operator*() const { return *ptr_; }

  T* get() { return ptr_.get(); }
  const T* get() const { return ptr_.get(); }

  T* operator->() { return ptr_.get(); }
  const T* operator->() const { return ptr_.get(); }
  //  shared_ptr<T> get_shared_ptr();

  bool operator==(const External& other) const { return ptr_ == other.ptr_; }
  bool operator!=(const External& other) const { return ptr_ != other.ptr_; }
  bool operator<(const External& other) const { return ptr_ < other.ptr_; }
  operator bool() const { return ptr_; }
  //  bool is_locked() const;
  //  void lock();

private:
  DCPS::unique_ptr<T> ptr_;
};

template <typename T>
External<T>::External()
  : ptr_(new T)
{}

template <typename T>
External<T>::External(const T& value)
  : ptr_(new T(value))
{}

template <typename T>
External<T>::External(T* p, bool)
  : ptr_(p)
{}

template <typename T>
External<T>::External(const External& other)
  : ptr_(new T(*other))
{}

template <typename T>
External<T>& External<T>::operator=(const External& other)
{
  ptr_.reset(new T(*other));
  return *this;
}

template <typename T>
External<T>& External<T>::operator=(const T& value)
{
  ptr_.reset(new T(value));
  return *this;
}

} // namespace OpenDDS
} // namespace XTypes

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_XTYPES_EXTERNAL_H
