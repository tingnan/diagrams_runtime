// Copyright 2015 Native Client Authors.

#ifndef RUNTIME_DRAW_GL_UTILITY_H
#define RUNTIME_DRAW_GL_UTILITY_H

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#include <vector>

#include "include/matrix_types.h"
#include "physics/node.h"

namespace diagrammar {

struct GLProgram {
  GLuint pid;
  // the user provided model_view_projection matrix
  GLuint u_mvp;
  GLuint color;
  GLuint vertex;
  GLuint normal;
  GLuint texture;
  GLuint resolution;
};

// We set the dimension of vertices (x, y, z, w), normals (x, y, z, padding) and
// colors(r, g, b, a) to be 4.
const static GLuint kDiagrammarGLVertexDimension = 4;

struct GLTriangleMesh {
  GLTriangleMesh() = default;
  GLTriangleMesh(GLuint num_vertices);
  GLTriangleMesh(GLuint num_vertices, GLuint num_triangles);
  std::vector<GLfloat> vertices;
  std::vector<GLfloat> normals;
  std::vector<GLfloat> colors;
  std::vector<GLuint> indices;
};

// Convert a TriangleMesh2D into OpenGL mesh that lies in a plane parallel to
// x-y, with offset depth; the normal direction by default is along positive z
// direction.
GLTriangleMesh ConvertDiagMesh2DToGLMesh(const TriangleMesh2D &diag_mesh,
                                         GLfloat depth, bool normal_up = true);

GLTriangleMesh CombineGLMesh(std::vector<GLTriangleMesh> meshes);

// TODO(tingnan)
// Make a OpenGL mesh from a Path2D using sweeping method. The sweep direction
// is z and the distance swept is given by depth. The last parameter indicates
// if the normals will point outward for a closed path.
GLTriangleMesh SweeptPath2DToGLMesh(const Path2D &path, GLfloat depth,
                                    bool is_closed = true, bool outward = true);
// TODO(tingnan)
// Make a OpenGL mesh from a Polygon2D object
GLTriangleMesh SweepPolygon2DToGLMesh(const Polygon2D &polygon, GLfloat depth);

// This method creates a GLTriangleMesh that lies in x-y plane, with normals
// pointing to positive-z direction.
// The dimension should be >=3
GLTriangleMesh GLTriangulate2DShape2D(const CollisionShape2D *shape_ptr);
// TODO(tingnan)
// This method will extend the 2D input shape into 3D hulls and then return the
// triangulation.
GLTriangleMesh GLTriangulate3DShape2D(const CollisionShape2D *shape_ptr,
                                      GLfloat depth);

std::vector<GLfloat> SerializePath2D(const Path2D &path, bool is_closed);

GLuint CreateGLProgram(const char *vert_shader_src,
                       const char *frag_shader_src);

}  // namespace diagrammar

#endif  // RUNTIME_DRAW_GL_UTILITY_H
