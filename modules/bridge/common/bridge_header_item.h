/******************************************************************************
 * Copyright 2019 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#pragma once

#include <string>

namespace apollo {
namespace bridge {

enum HType {
  Header_Ver,
  Msg_Name,
  Msg_ID,
  Msg_Size,
  Msg_Packs,
  Pack_Size,
  Msg_Index,
  Time_Stamp,

  Header_Tail,
};

class HeaderItemBase {
 public:
  HeaderItemBase() = default;
  virtual ~HeaderItemBase();

 public:
  virtual char *SerializeItem(char *buf, size_t buf_size,
    size_t *serialized_size) = 0;
  virtual const char *DiserializeItem(const char *buf,
    size_t *diserialized_size) = 0;
  virtual HType GetType() const = 0;
};

template <enum HType t, typename T>
struct HeaderItem;

template <enum HType t, typename T>
char *SerializeItemImp(const HeaderItem<t, T> &item, char *buf, size_t buf_size,
  size_t *serialized_size) {
  if (!buf || buf_size == 0 || !serialized_size ||
    buf_size < size_t(sizeof(t) + item.ValueSize() + 2) ) {
    return nullptr;
  }
  char *p = buf;
  size_t item_size = item.ValueSize();

  HType t1 = t;
  memcpy(p, &t1, sizeof(HType));
  p[sizeof(HType)] = ':';
  p = p + sizeof(HType) + 1;
  *serialized_size += sizeof(HType) + 1;

  memcpy(p, &item_size, sizeof(size_t));
  p[sizeof(size_t)] = ':';
  p = p + sizeof(size_t) + 1;
  *serialized_size += sizeof(size_t) + 1;

  memcpy(p, item.GetValuePtr(), item.ValueSize());
  p[item.ValueSize()] = '\n';
  p += item.ValueSize() + 1;
  *serialized_size += item.ValueSize() + 1;
  return p;
}

template <enum HType t, typename T>
const char *DiserializeItemImp(HeaderItem<t, T> *item, const char *buf,
    size_t *diserialized_size) {
  if (!buf || !diserialized_size) {
    return nullptr;
  }
  const char *p = buf;

  char p_type[sizeof(HType)] = {0};
  memcpy(p_type, buf, sizeof(HType));
  HType type = *(reinterpret_cast<HType *>(p_type));
  if (type != t) {
    return nullptr;
  }
  p += sizeof(HType) + 1;
  *diserialized_size += sizeof(HType) + 1;

  char p_size[sizeof(size_t)] = {0};
  memcpy(p_size, p, sizeof(size_t));
  size_t size = *(reinterpret_cast<size_t *>(p_size));
  p += sizeof(size_t) + 1;
  *diserialized_size += sizeof(size_t) + 1;

  item->SetValue(p);
  p += size + 1;
  *diserialized_size += size + 1;
  return p;
}

template <enum HType t, typename T>
struct HeaderItem: public HeaderItemBase {
  T value_;

  operator T () { return value_; }
  HeaderItem  &operator = (const T &val) {
    value_ = val;
    return *this;
  }
  HType GetType() const override { return t; }
  size_t ValueSize() const { return sizeof(value_); }
  const T *GetValuePtr() const { return &value_; }
  void SetValue(const char *buf) {
    if (!buf) {
      return;
    }
    value_ = *(reinterpret_cast<const T *>(buf));
  }

  char *SerializeItem(char *buf, size_t buf_size,
    size_t *serialized_size) override {
    return SerializeItemImp(*this, buf, buf_size, serialized_size);
  }

  const char *DiserializeItem(const char *buf,
    size_t *diserialized_size) override {
    return DiserializeItemImp(this, buf, diserialized_size);
  }
};

template <enum HType t>
struct HeaderItem<t, std::string> : public HeaderItemBase {
  std::string value_;
  operator std::string () { return value_; }
  HeaderItem  &operator = (const std::string &val) {
    value_ = val;
    return *this;
  }
  size_t ValueSize() const { return value_.length() + 1; }
  HType GetType() const override { return t; }
  const char *GetValuePtr() const { return value_.c_str(); }
  void SetValue(const char *buf) {
    if (!buf) {
      return;
    }
    value_ = std::string(buf);
  }

  char *SerializeItem(char *buf, size_t buf_size,
    size_t *serialized_size) override {
    return SerializeItemImp(*this, buf, buf_size, serialized_size);
  }

  const char *DiserializeItem(const char *buf,
    size_t *diserialized_size) override {
    return DiserializeItemImp(this, buf, diserialized_size);
  }
};

}  // namespace bridge
}  // namespace apollo
