/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2017                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef CAF_UPSTREAM_MSG_HPP
#define CAF_UPSTREAM_MSG_HPP

#include <utility>
#include <vector>
#include <cstdint>

#include "caf/actor_addr.hpp"
#include "caf/actor_control_block.hpp"
#include "caf/atom.hpp"
#include "caf/message.hpp"
#include "caf/stream_priority.hpp"
#include "caf/stream_slot.hpp"
#include "caf/variant.hpp"

#include "caf/tag/boxing_type.hpp"

#include "caf/detail/type_list.hpp"

namespace caf {

/// Stream messages that travel upstream, i.e., acks and drop messages.
struct upstream_msg : tag::boxing_type {
  // -- nested types -----------------------------------------------------------

  /// Acknowledges a previous `open` message and finalizes a stream handshake.
  /// Also signalizes initial demand.
  struct ack_open {
    /// Allows the testing DSL to unbox this type automagically.
    using outer_type = upstream_msg;

    /// Allows actors to participate in a stream instead of the actor
    /// originally receiving the `open` message. No effect when set to
    /// `nullptr`. This mechanism enables pipeline definitions consisting of
    /// proxy actors that are replaced with actual actors on demand.
    actor_addr rebind_from;

    /// Points to sender_, but with a strong reference.
    strong_actor_ptr rebind_to;

    /// Grants credit to the source.
    int32_t initial_demand;

    /// Desired size of individual batches.
    int32_t desired_batch_size;

    /// Tells the upstream whether rebindings can occur on this path.
    bool redeployable;
  };

  /// Cumulatively acknowledges received batches and signalizes new demand from
  /// a sink to its source.
  struct ack_batch {
    /// Allows the testing DSL to unbox this type automagically.
    using outer_type = upstream_msg;

    /// Newly available credit.
    int32_t new_capacity;

    /// Desired size of individual batches for the next cycle.
    int32_t desired_batch_size;

    /// Cumulative ack ID.
    int64_t acknowledged_id;
  };

  /// Informs a source that a sink orderly drops out of a stream.
  struct drop {
    /// Allows the testing DSL to unbox this type automagically.
    using outer_type = upstream_msg;
  };

  /// Propagates a fatal error from sinks to sources.
  struct forced_drop {
    /// Allows the testing DSL to unbox this type automagically.
    using outer_type = upstream_msg;

    /// Reason for shutting down the stream.
    error reason;
  };

  // -- member types -----------------------------------------------------------

  /// Lists all possible options for the payload.
  using alternatives =
    detail::type_list<ack_open, ack_batch, drop, forced_drop>;

  /// Stores one of `alternatives`.
  using content_type = variant<ack_open, ack_batch, drop, forced_drop>;

  // -- constructors, destructors, and assignment operators --------------------

  template <class T>
  upstream_msg(stream_slot id, actor_addr addr, T&& x)
      : slot(id),
        sender(std::move(addr)),
        content(std::forward<T>(x)) {
    // nop
  }

  upstream_msg() = default;
  upstream_msg(upstream_msg&&) = default;
  upstream_msg(const upstream_msg&) = default;
  upstream_msg& operator=(upstream_msg&&) = default;
  upstream_msg& operator=(const upstream_msg&) = default;

  // -- member variables -------------------------------------------------------

  /// ID of the affected stream.
  stream_slot slot;

  /// Address of the sender. Identifies the up- or downstream actor sending
  /// this message. Note that abort messages can get send after `sender`
  /// already terminated. Hence, `current_sender()` can be `nullptr`, because
  /// no strong pointers can be formed any more and receiver would receive an
  /// anonymous message.
  actor_addr sender;

  /// Palyoad of the message.
  content_type content;
};

/// Allows the testing DSL to unbox `stream_msg` automagically.
template <class T>
const T& get(const upstream_msg& x) {
  return get<T>(x.content);
}

/// Allows the testing DSL to check whether `stream_msg` holds a `T`.
template <class T>
bool is(const upstream_msg& x) {
  return holds_alternative<T>(x.content);
}

/// @relates upstream_msg
template <class T, class... Ts>
detail::enable_if_tt<detail::tl_contains<upstream_msg::alternatives, T>,
                     upstream_msg>
make(stream_slot slot, actor_addr addr, Ts&&... xs) {
  return {slot, std::move(addr), T{std::forward<Ts>(xs)...}};
}

/// @relates upstream_msg::ack_open
template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        upstream_msg::ack_open& x) {
  return f(meta::type_name("ack_open"), x.rebind_from, x.rebind_to,
           x.initial_demand, x.desired_batch_size, x.redeployable);
}

/// @relates upstream_msg::ack_batch
template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        upstream_msg::ack_batch& x) {
  return f(meta::type_name("ack_batch"), x.new_capacity, x.desired_batch_size,
           x.acknowledged_id);
}

/// @relates upstream_msg::drop
template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, upstream_msg::drop&) {
  return f(meta::type_name("drop"));
}

/// @relates upstream_msg::forced_drop
template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        upstream_msg::forced_drop& x) {
  return f(meta::type_name("forced_drop"), x.reason);
}

/// @relates upstream_msg
template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, upstream_msg& x) {
  return f(meta::type_name("stream_msg"), x.slot, x.sender, x.content);
}

} // namespace caf

#endif // CAF_UPSTREAM_MSG_HPP