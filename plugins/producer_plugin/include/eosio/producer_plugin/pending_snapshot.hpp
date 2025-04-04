#pragma once

#include <eosio/producer_plugin/producer_plugin.hpp>

namespace eosio {

class pending_snapshot {
public:
   using next_t = producer_plugin::next_function<producer_plugin::snapshot_information>;

   pending_snapshot(const chain::block_id_type& block_id, next_t& next, std::string pending_path, std::string final_path)
         : block_id(block_id)
         , next(next)
         , pending_path(pending_path)
         , final_path(final_path)
   {}

   uint32_t get_height() const {
      return chain::block_header::num_from_id(block_id);
   }

   static std::filesystem::path get_final_path(const chain::block_id_type& block_id, const std::filesystem::path& snapshots_dir) {
      return snapshots_dir / fc::format_string("snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
   }

   static std::filesystem::path get_pending_path(const chain::block_id_type& block_id, const std::filesystem::path& snapshots_dir) {
      return snapshots_dir / fc::format_string(".pending-snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
   }

   static std::filesystem::path get_temp_path(const chain::block_id_type& block_id, const std::filesystem::path& snapshots_dir) {
      return snapshots_dir / fc::format_string(".incomplete-snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
   }

   producer_plugin::snapshot_information finalize( const chain::controller& chain ) const;

   chain::block_id_type     block_id;
   next_t                   next;
   std::string              pending_path;
   std::string              final_path;
};

} // namespace eosio
