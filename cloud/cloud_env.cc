// Copyright (c) 2017 Rockset.
#ifndef ROCKSDB_LITE

#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "cloud/aws/aws_env.h"
#include "cloud/db_cloud_impl.h"
#include "cloud/filename.h"

namespace rocksdb {

CloudEnvImpl::CloudEnvImpl(CloudType type, Env* base_env) :
  cloud_type_(type),
  is_cloud_direct_(false),
  is_clone_(false),
  base_env_(base_env),
  purger_is_running_(true) {
}

CloudEnvImpl::~CloudEnvImpl() {
  // tell the purger to stop
  purger_is_running_ = false;

  // wait for the purger to stop
  if (purge_thread_.joinable()) {
    purge_thread_.join();
  }
}

Status CloudEnv::NewAwsEnv(Env* base_env,
		           const std::string& src_cloud_storage,
                           const std::string& src_cloud_object_prefix,
                           const std::string& dest_cloud_storage,
                           const std::string& dest_cloud_object_prefix,
	                   const CloudEnvOptions& options,
			   std::shared_ptr<Logger> logger,
			   CloudEnv** cenv) {

  Status st = AwsEnv::NewAwsEnv(base_env,
		                src_cloud_storage, src_cloud_object_prefix,
				dest_cloud_storage, dest_cloud_object_prefix,
			        options, logger, cenv);
  if (st.ok()) {
    // store a copy of the logger
    CloudEnvImpl* cloud = static_cast<CloudEnvImpl *>(*cenv);
    cloud->info_log_ = logger;

    // start the purge thread
    cloud->purge_thread_ = std::thread([cloud] { cloud->Purger(); });
  }
  return st;
}

//
// Marks this env as a clone env
//
Status CloudEnvImpl::SetClone(const std::string& src_dbid) {
  is_clone_ = true;
  src_dbid_ = src_dbid;

  // Map the src dbid to a pathname of the src db
  // src db dir
  return GetPathForDbid(src_dbid, &src_dbdir_);
}

}  // namespace rocksdb
#endif  // ROCKSDB_LITE
