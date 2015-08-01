// Copyright 2015 Native Client Authors.

#include <vector>
#include <string>
#include <iostream>

#include "utility/world_parser.h"
#include "draw/gl_utility.h"
#include "physics/node.h"

namespace {
const char kFragShaderSource[] = "precision mediump float;\n"
                                 "varying vec4 v_color;\n"
                                 "void main() {\n"
                                 "  gl_FragColor = v_color;\n"
                                 "}\n";

const char kVertShaderSource[] =
    "uniform mat4 u_mvp;\n"
    "attribute vec4 vertex;\n"
    "attribute vec4 normal;\n"
    "attribute vec4 color;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_Position = u_mvp * vec4(vertex.xyz, 1.0);\n"
    "  v_color = vec4(color.xyz, 1.0);\n"
    "}\n";

} // namespace

namespace diagrammar {

void ShaderErrorHandler(GLuint shader_id) {
  GLint log_size;
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_size);
  log_size = log_size > 0 ? log_size : 1024;
  std::vector<char> compileLog(log_size);
  glGetShaderInfoLog(shader_id, log_size, nullptr, compileLog.data());
  std::cerr << "Compile Error: " << shader_id << " " << compileLog.data()
            << std::endl;
}

void ProgarmErrorHanlder(GLuint program_id) {
  GLint log_size;
  glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_size);
  log_size = log_size > 0 ? log_size : 1024;
  std::vector<char> errorMessage(log_size);
  glGetProgramInfoLog(program_id, log_size, nullptr, errorMessage.data());
  std::cerr << "Linking Error: " << errorMessage.data() << std::endl;
}

GLuint CompileShaderFromSource(const char *data, GLenum type) {
  GLuint shader_id = glCreateShader(type);
  glShaderSource(shader_id, 1, &data, nullptr);
  glCompileShader(shader_id);
  GLint result = GL_FALSE;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
  if (result != GL_TRUE) {
    ShaderErrorHandler(shader_id);
    return 0;
  }
  return shader_id;
}

GLuint CompileShaderFromFile(const char *fname, GLenum type) {
  std::string shader_text = diagrammar::Stringify(fname);
  const char *shader_text_cstr = shader_text.c_str();
  return CompileShaderFromSource(shader_text_cstr, type);
}

GLuint CreateGLProgram(const char *vert_shader_src,
                       const char *frag_shader_src) {
  GLuint vert_shader_id =
      CompileShaderFromSource(vert_shader_src, GL_VERTEX_SHADER);
  GLuint frag_shader_id =
      CompileShaderFromSource(frag_shader_src, GL_FRAGMENT_SHADER);
  // now we can link the program
  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vert_shader_id);
  glAttachShader(program_id, frag_shader_id);
  glLinkProgram(program_id);
  GLint result;
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  if (result != GL_TRUE) {
    ProgarmErrorHanlder(program_id);
  }
  glDeleteShader(vert_shader_id);
  glDeleteShader(frag_shader_id);

  return program_id;
}

GLProgram LoadDefaultGLProgram() {
  GLProgram program;
  program.pid = CreateGLProgram(kVertShaderSource, kFragShaderSource);
  program.u_mvp = glGetUniformLocation(program.pid, "u_mvp");
  program.color = glGetAttribLocation(program.pid, "color");
  program.normal = glGetAttribLocation(program.pid, "normal");
  program.vertex = glGetAttribLocation(program.pid, "vertex");
  return program;
}

GLTriangleMesh::GLTriangleMesh(GLuint num_vertices, GLuint num_faces)
    : vertices(dimension * num_vertices), normals(dimension * num_vertices),
      colors(dimension * num_vertices), indices(3 * num_faces) {}

GLTriangleMesh ConvertDiagMesh2DToGLMesh(const TriangleMesh2D &diag_mesh,
                                         GLfloat depth, bool normal_up) {
  int normal_sign = normal_up ? 1 : -1;
  GLTriangleMesh gl_mesh(diag_mesh.vertices.size(), diag_mesh.faces.size());
  const size_t num_vertices = diag_mesh.vertices.size();
  for (size_t i = 0, j = 0; i < num_vertices; ++i, j += 4) {
    gl_mesh.vertices[j + 0] = diag_mesh.vertices[i](0);
    gl_mesh.vertices[j + 1] = diag_mesh.vertices[i](1);
    gl_mesh.vertices[j + 2] = depth;
    gl_mesh.vertices[j + 3] = 1;

    gl_mesh.normals[j + 0] = 0;
    gl_mesh.normals[j + 1] = 0;
    gl_mesh.normals[j + 2] = normal_sign;
    gl_mesh.normals[j + 3] = 1;

    if (i % 3 == 0) {
      // r
      gl_mesh.colors[j + 0] = 1;
      gl_mesh.colors[j + 1] = 0;
      gl_mesh.colors[j + 2] = 0;
      gl_mesh.colors[j + 3] = 1;
    }

    if (i % 3 == 1) {
      // g
      gl_mesh.colors[j + 0] = 0;
      gl_mesh.colors[j + 1] = 1;
      gl_mesh.colors[j + 2] = 0;
      gl_mesh.colors[j + 3] = 1;
    }

    if (i % 3 == 2) {
      // b
      gl_mesh.colors[j + 0] = 0;
      gl_mesh.colors[j + 1] = 0;
      gl_mesh.colors[j + 2] = 1;
      gl_mesh.colors[j + 3] = 1;
    }
  }

  const size_t num_faces = diag_mesh.faces.size();
  for (size_t i = 0, j = 0; i < num_faces; ++i, j += 3) {
    gl_mesh.indices[j + 0] = diag_mesh.faces[i].at(0);
    gl_mesh.indices[j + 1] = diag_mesh.faces[i].at(1);
    gl_mesh.indices[j + 2] = diag_mesh.faces[i].at(2);
  }

  return gl_mesh;
}

GLTriangleMesh CombineGLMesh(std::vector<GLTriangleMesh> meshes) {
  GLTriangleMesh combined_mesh;
  // The only thing we need to worry about is the indices array;
  for (auto &mesh : meshes) {
    // We MUST insert at the end, to correctly use index offset
    size_t offset = combined_mesh.vertices.size() / 4;
    combined_mesh.vertices.insert(
        combined_mesh.vertices.end(),
        std::make_move_iterator(mesh.vertices.begin()),
        std::make_move_iterator(mesh.vertices.end()));
    combined_mesh.normals.insert(combined_mesh.normals.end(),
                                 std::make_move_iterator(mesh.normals.begin()),
                                 std::make_move_iterator(mesh.normals.end()));
    combined_mesh.colors.insert(combined_mesh.colors.end(),
                                std::make_move_iterator(mesh.colors.begin()),
                                std::make_move_iterator(mesh.colors.end()));
    // Update the mesh indices before inserting to the back
    for (auto &idx : mesh.indices) {
      idx += offset;
    }
    combined_mesh.indices.insert(combined_mesh.indices.end(),
                                 std::make_move_iterator(mesh.indices.begin()),
                                 std::make_move_iterator(mesh.indices.end()));
  }
  return combined_mesh;
}

GLTriangleMesh GLTriangulate2DShape2D(CollisionShape2D *shape_ptr,
                                      GLuint dimension) {
  switch (shape_ptr->shape_type) {
  case Shape2DType::kDisk: {
    auto sphere_ptr = dynamic_cast<Disk2D *>(shape_ptr);
    const size_t num_vertices = 30;
    Polygon2D polygon;
    for (size_t i = 0; i < num_vertices; ++i) {
      float theta = float(i) / num_vertices * M_PI * 2;
      polygon.path.emplace_back(cos(theta) * sphere_ptr->radius,
                                sin(theta) * sphere_ptr->radius);
    }
    auto diag_mesh = TriangulatePolygon(polygon);
    return ConvertDiagMesh2DToGLMesh(diag_mesh, 0.0);
  } break;
  case Shape2DType::kPolygon: {
    auto poly_ptr = dynamic_cast<Polygon2D *>(shape_ptr);
    auto diag_mesh = TriangulatePolygon(*poly_ptr);
    return ConvertDiagMesh2DToGLMesh(diag_mesh, 0.0);
  } break;
  case Shape2DType::kPolyLine: {
    auto line_ptr = dynamic_cast<Line2D *>(shape_ptr);
    auto diag_mesh = TriangulatePolyline(line_ptr->path, 1.5);
    return ConvertDiagMesh2DToGLMesh(diag_mesh, 0.0);
  } break;
  default:
    break;
  }
  return GLTriangleMesh();
}

GLTriangleMesh GLTriangulate3DShape2D(struct CollisionShape2D *shape_ptr,
                                      GLfloat depth) {
  switch (shape_ptr->shape_type) {
  case Shape2DType::kDisk: {
    auto sphere_ptr = dynamic_cast<Disk2D *>(shape_ptr);
    const size_t num_vertices = 30;
    Polygon2D polygon;
    for (size_t i = 0; i < num_vertices; ++i) {
      float theta = float(i) / num_vertices * M_PI * 2;
      polygon.path.emplace_back(cos(theta) * sphere_ptr->radius,
                                sin(theta) * sphere_ptr->radius);
    }
    auto diag_mesh = TriangulatePolygon(polygon);
  } break;
  case Shape2DType::kPolygon: {
    auto poly_ptr = dynamic_cast<Polygon2D *>(shape_ptr);
    auto diag_mesh = TriangulatePolygon(*poly_ptr);
  } break;
  case Shape2DType::kPolyLine: {
    auto line_ptr = dynamic_cast<Line2D *>(shape_ptr);
    auto diag_mesh = TriangulatePolyline(line_ptr->path, 1.5);
  } break;
  default:
    break;
  }
  return GLTriangleMesh();
}

} // namespace diagrammar
