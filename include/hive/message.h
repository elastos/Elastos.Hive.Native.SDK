
#if !defined(__DMMESSAGE_H_)
#define __DMMESSAGE_H_

#include <vector>

#include <hive/node.h>
#include <string>

class DMessage {
 public:
  DMessage(const std::string& timestamp, const std::string& message) {
    name = timestamp;
    content = message;
  }

  std::string timestamp() { return name; }

  std::string value() { return content; }

 private:
  std::string name;
  std::string content;
};

typedef struct dstore_node {
  std::string ipv4;
  std::string ipv6;
  uint16_t port;
} dstore_node;

class DStore {
 public:
  explicit DStore(std::vector<dstore_node> &&nodes, bool log = false);

  std::string get_peerId();

  std::shared_ptr<std::vector<std::string>> get_keys();

  std::shared_ptr<std::vector<std::string>> get_remote_keys(
      const std::string& peerId);

  std::shared_ptr<std::vector<std::shared_ptr<DMessage>>> get_values(
      const std::string& key);

  std::shared_ptr<std::vector<std::shared_ptr<DMessage>>> get_remote_values(
      const std::string& peerId, const std::string& key);

  bool add_value(const std::string& key, std::shared_ptr<DMessage> message);

  bool remove_values(const std::string& key);

  void enable_log(bool enable);

 private:
  bool set_sender_UID(std::string& uid);

  std::shared_ptr<std::vector<std::shared_ptr<DMessage>>> get_values_internal(
      const std::string& key);

  bool add_value_internal(const std::string& key, std::shared_ptr<DMessage> message);

  bool remove_values_internal(const std::string& key);

  bool select_node();

  bool publish();

  bool resolve();

 private:
  ipfs::Node node;
  std::string userId;
  std::string peerId;
  std::string rootPath;
  std::string workingPath;
  bool needPublish;
  bool needResolve;
  bool debugLog;
  std::vector<dstore_node> candidate_nodes;
};

#endif  // __DMMESSAGE_H_
