// Copyright (c) 2013, Cloudera, inc.
// Confidential Cloudera Information: Covered by NDA.

#include <boost/assign/list_of.hpp>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "kudu/fs/block_manager.h"
#include "kudu/fs/fs_manager.h"
#include "kudu/gutil/strings/util.h"
#include "kudu/util/test_macros.h"
#include "kudu/util/test_util.h"

namespace kudu {

class FsManagerTestBase : public KuduTest {
 public:
  void SetUp() OVERRIDE {
    KuduTest::SetUp();

    // Initialize File-System Layout
    fs_manager_.reset(new FsManager(env_.get(), test_dir_));
    ASSERT_STATUS_OK(fs_manager_->CreateInitialFileSystemLayout());
    ASSERT_STATUS_OK(fs_manager_->Open());
  }

  void TestReadWriteDataFile(const Slice& data) {
    uint8_t buffer[64];
    DCHECK_LT(data.size(), sizeof(buffer));

    // Test Write
    gscoped_ptr<fs::WritableBlock> writer;
    ASSERT_STATUS_OK(fs_manager()->CreateNewBlock(&writer));
    ASSERT_STATUS_OK(writer->Append(data));
    ASSERT_STATUS_OK(writer->Close());

    // Test Read
    Slice result;
    gscoped_ptr<fs::ReadableBlock> reader;
    ASSERT_STATUS_OK(fs_manager()->OpenBlock(writer->id(), &reader));
    ASSERT_STATUS_OK(reader->Read(0, data.size(), &result, buffer));
    ASSERT_EQ(data.size(), result.size());
    ASSERT_EQ(0, result.compare(data));
  }

  FsManager *fs_manager() const { return fs_manager_.get(); }

 private:
  gscoped_ptr<FsManager> fs_manager_;
};

TEST_F(FsManagerTestBase, TestBaseOperations) {
  fs_manager()->DumpFileSystemTree(std::cout);

  TestReadWriteDataFile(Slice("test0"));
  TestReadWriteDataFile(Slice("test1"));

  fs_manager()->DumpFileSystemTree(std::cout);
}

TEST_F(FsManagerTestBase, TestWhitespaceInPaths) {
  gscoped_ptr<FsManager> new_fs_manager(new FsManager(env_.get(),
                                                      "  /foo\n\t"));
  ASSERT_TRUE(HasPrefixString(new_fs_manager->GetDataRootDir(), "/foo/"));
  ASSERT_TRUE(HasPrefixString(new_fs_manager->GetWalsRootDir(), "/foo/"));
}

TEST_F(FsManagerTestBase, TestMultiplePaths) {
  string wal_path = GetTestPath("a");
  vector<string> data_paths = boost::assign::list_of(
      GetTestPath("a"))(GetTestPath("b"))(GetTestPath("c"));
  gscoped_ptr<FsManager> new_fs_manager(new FsManager(env_.get(), NULL,
                                                      wal_path, data_paths));
  ASSERT_OK(new_fs_manager->CreateInitialFileSystemLayout());
  ASSERT_OK(new_fs_manager->Open());
}

} // namespace kudu
