#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/vm/host_function.hpp>
#include <eosio/vm/reference_proxy.hpp>
#include <eosio/vm/span.hpp>
#include <eosio/vm/types.hpp>

using namespace fc;

namespace eosio { namespace chain { namespace webassembly {
   // forward declaration
   class interface;
}}} // ns eosio::chain::webassembly

// TODO need to fix up wasm_tests to not use this macro
#define EOSIO_INJECTED_MODULE_NAME "eosio.injection"

namespace eosio { namespace chain {

   class apply_context;
   class transaction_context;

   inline static constexpr auto eosio_injected_module_name = EOSIO_INJECTED_MODULE_NAME;


   template <typename T>
   using legacy_ptr = eosio::vm::reference_proxy<T, true>;

   template <typename T>
   using legacy_array_ptr = eosio::vm::reference_proxy<eosio::vm::span<T>>;

   struct null_terminated_ptr : eosio::vm::span<const char> {
      using base_type = eosio::vm::span<const char>;
      null_terminated_ptr(const char* ptr) : base_type(ptr, strlen(ptr)) {}
   };

   // wrapper pointer type to keep the validations from being done
   template <typename T>
   struct unvalidated_ptr {
      static_assert(sizeof(T) == 1); // currently only going to support these for char and const char
      operator T*() { return ptr; }
      T* operator->() { return ptr; }
      T& operator*() { return *ptr; }
      T* ptr;
   };

   // define the type converter for eosio
   struct type_converter : public eosio::vm::type_converter<webassembly::interface, eosio::vm::execution_interface> {
      using base_type = eosio::vm::type_converter<webassembly::interface, eosio::vm::execution_interface>;
      using base_type::type_converter;
      using base_type::from_wasm;
      using base_type::to_wasm;
      using base_type::as_value;
      using base_type::as_result;
      using base_type::elem_type;
      using base_type::get_host;

      template <typename T>
      auto from_wasm(const elem_type& ptr) const
         -> std::enable_if_t<std::is_same_v<T, unvalidated_ptr<const char>>, T> {
         return {as_value<const char*>(ptr)};
      }

      template <typename T>
      auto from_wasm(const elem_type& ptr) const
         -> std::enable_if_t<std::is_same_v<T, unvalidated_ptr<char>>, T> {
         return {as_value<char*>(ptr)};
      }

#if 0
      template <typename T, typename U>
      auto from_wasm(U* ptr) const -> std::enable_if_t<std::is_same_v<T, U*>, eosio::vm::reference_proxy<T>> {
         return {ptr};
      }

      template <typename T, typename U>
      auto from_wasm(eosio::vm::reference<U> r) const
         -> std::enable_if_t<std::is_same_v<T, eosio::vm::reference<U>>, eosio::vm::reference_proxy<T>> {
         return {r.get()};
      }
#endif

      EOS_VM_FROM_WASM(null_terminated_ptr, (const elem_type& ptr)) { return {as_value<const char*>(ptr)}; }
      EOS_VM_FROM_WASM(name, (const elem_type& e)) { return name{static_cast<uint64_t>(as_value<uint64_t>(e))}; }
      EOS_VM_TO_WASM(name, (name n)) { return as_result<uint64_t>(n.to_uint64_t()); }
      // OSX clang can't find the base class version
      EOS_VM_FROM_WASM(bool, (const elem_type& value)) { return as_value<uint32_t>(value) ? 1 : 0; }
   };

   using eos_vm_host_functions_t = eosio::vm::registered_host_functions<webassembly::interface,
                                                                        eosio::vm::execution_interface,
                                                                        eosio::chain::type_converter>;
   using wasm_size_t = eosio::vm::wasm_size_t;

}} // eosio::chain
