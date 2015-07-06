// Copyright 2015 Native Client Authors

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <json/json.h>
#include "utility/world_parser.h"
#include "geometry/aabb.h"

namespace diagrammar {
std::string Stringify(const char* path) {
  std::ifstream file_handle(path, std::ios::binary | std::ios::in);
  std::string text;
  if (file_handle.is_open()) {
    file_handle.seekg(0, std::ios::end);
    text.resize(file_handle.tellg());
    file_handle.seekg(0, std::ios::beg);
    file_handle.read(&text[0], text.size());
    file_handle.close();
  } else {
    std::cout << "cannot open file\n";
  }
  return text;
}

Json::Value CreateJsonObject(const char* file) {
  std::string content = diagrammar::Stringify(file);
  Json::Reader reader;
  Json::Value json_obj;
  bool success = reader.parse(content, json_obj);
  if (!success) {
    std::cout << "not a valid json file" << std::endl;
    exit(-1);
  }
  return json_obj;
}

Isometry2f ParseTransformation2D(const Json::Value& array) {
  Isometry2f t = Isometry2f::Identity();
  assert(array.size() == 6);

  Matrix2f rot;
  rot << array[0].asFloat(), array[2].asFloat(), array[1].asFloat(),
      array[3].asFloat();
  // rotate then translate, the order is important;
  t.translate(Vector2f(array[4].asFloat(), array[5].asFloat())).rotate(rot);
  return t;
}

Polyline ParsePath2D(const Json::Value& pathobj) {
  Polyline mypath;

  Json::Value::const_iterator itr = pathobj.begin();
  for (; itr != pathobj.end(); ++itr) {
    const Json::Value& pt = *itr;
    float x = pt.get("x", 0).asFloat();
    float y = pt.get("y", 0).asFloat();
    mypath.emplace_back(Vector2f(x, y));
  }
  return mypath;
}

// load a "children" node from the json descriptor
Node ParseNode(const Json::Value& nodeobj) {
  Node node;
  if (!nodeobj.isMember("type")) {
    return node;
  }
  std::string ntype = nodeobj["type"].asString();
  if (ntype != "node" && ntype != "open_path") {
    return node;
  }

  Json::Value::const_iterator itr = nodeobj.begin();
  for (; itr != nodeobj.end(); ++itr) {
    if (itr.key().asString() == "id") {
      node.set_id((*itr).asInt());
    }

    if (itr.key().asString() == "transform") {
      const Isometry2f tr = ParseTransformation2D(*itr);
      node.SetRotationMatrix(tr.linear());
      node.SetPosition(tr.translation());
    }

    if (itr.key().asString() == "path") {
      if (ntype == "node") {
        node.AddGeometry(Polygon(ParsePath2D(*itr)));
      }
      if (ntype == "open_path") {
        node.AddGeometry(ParsePath2D(*itr));
      }
    }

    if (itr.key().asString() == "inner_path") {
      const std::vector<Vector2f>& path = ParsePath2D(*itr);
      AABB bounding_box = GetAABBWithPadding(path, 5e-2);
      std::vector<Vector2f> box;
      Vector2f pt0 = bounding_box.lower_bound;
      Vector2f pt2 = bounding_box.upper_bound;
      Vector2f pt1(pt2(0), pt0(1));
      Vector2f pt3(pt0(0), pt2(1));
      box.emplace_back(pt0);
      box.emplace_back(pt1);
      box.emplace_back(pt2);
      box.emplace_back(pt3);
      Polygon geo(box);
      geo.holes.emplace_back(path);
      node.AddGeometry(std::move(geo));
    }
  }
  return node;
}
}  // namespace diagrammar
