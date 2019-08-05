#include <hive/c-api.h>
#include <hive/node.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <hive/c-api.h>
#include <hive/message.h>

DStoreC *dstore_create(dstorec_node *bootstraps, size_t sz) {
  try {
    std::vector<dstore_node> ds_bootstraps;

    if (!bootstraps || !sz)
      return nullptr;

    for (size_t i = 0; i < sz; ++i) {
      ds_bootstraps.push_back(dstore_node {bootstraps[i].ipv4,
                                           bootstraps[i].ipv6,
                                           bootstraps[i].port});
    }

    auto ds = new DStore{std::move(ds_bootstraps)};

    return reinterpret_cast<DStoreC *>(ds);
  } catch (...) {
    return nullptr;
  }
}

void dstore_destroy(DStoreC *dstore) {
  delete reinterpret_cast<DStore *>(dstore);
}

int dstore_get_values(DStoreC *dstore, const char *key,
                      bool (*callback)(const char *key, const uint8_t *value,
                                       size_t length, void *context),
                      void *context) {
  auto ds = reinterpret_cast<DStore *>(dstore);

  if (!dstore || !key || !*key || !callback) return -1;

  try {
    const auto dmsgs = ds->get_values(key);
    if (!dmsgs)
        return -1;
    for (auto &dmsg : *dmsgs) {
      auto value = dmsg->value();
      bool cont = callback(key, reinterpret_cast<const uint8_t *>(value.data()),
                           value.size(), context);
      if (!cont) break;
    }
    return 0;
  } catch (...) {
    return -1;
  }
}

int dstore_add_value(DStoreC *dstore, const char *key, const uint8_t *value,
                     size_t len) {
  static int cnt = 0;
  char buf[128];
  auto ds = reinterpret_cast<DStore *>(dstore);

  if (!dstore || !key || !*key || !value || !len) return -1;

  try {
    snprintf(buf, sizeof(buf), "%d", cnt++);
    auto dmsg = std::make_shared<DMessage>(
        buf, std::string{reinterpret_cast<const char *>(value), len});
    return ds->add_value(key, dmsg) ? 0 : -1;
  } catch (...) {
    return -1;
  }
}

int dstore_remove_values(DStoreC *dstore, const char *key) {
  auto ds = reinterpret_cast<DStore *>(dstore);

  if (!dstore || !key || !*key) return -1;

  try {
    return ds->remove_values(key) ? 0 : -1;
  } catch (...) {
    return -1;
  }
}
