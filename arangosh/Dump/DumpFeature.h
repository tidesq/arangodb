////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_DUMP_DUMP_FEATURE_H
#define ARANGODB_DUMP_DUMP_FEATURE_H 1

#include "ApplicationFeatures/ApplicationFeature.h"
#include "Utils/ClientManager.hpp"
#include "Utils/ClientTaskQueue.hpp"
#include "Utils/FileHandler.hpp"

namespace arangodb {
namespace httpclient {
class SimpleHttpResult;
}

class EncryptionFeature;

struct DumpFeatureStats {
  DumpFeatureStats(uint64_t b, uint64_t c, uint64_t w) noexcept;
  std::atomic<uint64_t> totalBatches;
  std::atomic<uint64_t> totalCollections;
  std::atomic<uint64_t> totalWritten;
};

struct DumpFeatureJobData {
  DumpFeatureJobData(VPackSlice const&, std::string const&, std::string const&,
                     std::string const&, uint64_t const&, uint64_t const&,
                     uint64_t const&, uint64_t const&, uint64_t const&,
                     bool const&, bool const&, std::string const&,
                     DumpFeatureStats&) noexcept;
  VPackSlice const& collectionInfo;
  std::string const cid;
  std::string const name;
  std::string const type;
  uint64_t batchId;
  uint64_t const tickStart;
  uint64_t const maxTick;
  uint64_t const initialChunkSize;
  uint64_t const maxChunkSize;
  bool const showProgress;
  bool const dumpData;
  std::string const& outputDirectory;
  DumpFeatureStats& stats;
};
extern template class ClientTaskQueue<DumpFeatureJobData>;

class DumpFeature final : public application_features::ApplicationFeature,
                          public ClientManager,
                          public ClientTaskQueue<DumpFeatureJobData>,
                          public FileHandler {
 public:
  DumpFeature(application_features::ApplicationServer* server, int* result);

 public:
  static std::string featureName();
  void collectOptions(std::shared_ptr<options::ProgramOptions>) override final;
  void validateOptions(
      std::shared_ptr<options::ProgramOptions> options) override final;
  void prepare() override final;
  void start() override final;

 private:
  std::vector<std::string> _collections;
  uint64_t _chunkSize;
  uint64_t _maxChunkSize;
  bool _dumpData;
  bool _force;
  bool _ignoreDistributeShardsLikeErrors;
  bool _includeSystemCollections;
  std::string _outputDirectory;
  bool _overwrite;
  bool _progress;
  uint64_t _tickStart;
  uint64_t _tickEnd;

 private:
  virtual Result processJob(httpclient::SimpleHttpClient& client,
                            DumpFeatureJobData& jobData) noexcept override;
  virtual void handleJobResult(std::unique_ptr<DumpFeatureJobData>&& jobData,
                               Result const& result) noexcept override;

 private:
  static Result dumpCollection(httpclient::SimpleHttpClient& client,
                               DumpFeatureJobData& jobData, int fd);
  static Result handleCollection(httpclient::SimpleHttpClient& client,
                                 DumpFeatureJobData& jobData);

  static Result dumpShard(httpclient::SimpleHttpClient& client,
                          DumpFeatureJobData& jobData, std::string const& DBserver, int fd, std::string const& shardName);
  static Result handleCollectionCluster(httpclient::SimpleHttpClient& client,
                            DumpFeatureJobData& jobData);

  void flushWal();
  int runDump(std::string& dbName, std::string& errorMsg);
  int runClusterDump(std::string& errorMsg);

 private:
  int* _result;
  uint64_t _batchId;
  bool _clusterMode;
  DumpFeatureStats _stats;
};
}  // namespace arangodb

#endif
