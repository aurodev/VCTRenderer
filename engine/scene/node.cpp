#include "node.h"

#include <glm/gtx/transform.hpp>

#include "mesh.h"
#include "../core/engine_base.h"
#include "../core/deferred_renderer.h"
#include "camera.h"

Node::Node() : name("Default Node")
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    scaling = glm::vec3(1.0f, 1.0f, 1.0f);
    rotation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
    GetModelMatrix(); // initially as invalid
}


Node::~Node()
{
}

glm::mat4x4 Node::GetModelMatrix() const
{
    return modelMatrix;
}

void Node::ComputeModelMatrix()
{
    modelMatrix = translate(position) * mat4_cast(rotation) * scale(scaling);
}

void Node::BuildDrawList(std::vector<Node *> &base)
{
    base.push_back(this);

    for (auto &node : nodes)
    {
        node->BuildDrawList(base);
    }
}

void Node::DrawMeshes()
{
    static const auto &renderer = EngineBase::Renderer();

    for (auto &mesh : meshes)
    {
        if (!mesh->IsLoaded()) { return; }

        if (meshes.size() > 1 && !Camera::Active()->InFrustum(mesh->boundaries))
        {
            continue;
        }

        renderer.SetMaterialUniforms(mesh->material);
        mesh->BindVertexArrayObject();
        mesh->DrawElements();
    }
}

void Node::Draw()
{
    // recalculate model-dependent transform matrices
    ComputeMatrices();
    // set this node as rendering active
    SetAsActive();

    if (!Camera::Active()->InFrustum(boundaries))
    {
        return;
    }

    EngineBase::Renderer().SetMatricesUniforms();
    // draw elements per mesh
    DrawMeshes();
}

void Node::DrawRecursive()
{
    // set this node as rendering active
    SetAsActive();

    if (!Camera::Active()->InFrustum(boundaries))
    {
        return;
    }

    // recalculate model-dependent transform matrices
    ComputeMatrices();
    // set matrices uniform with updated matrices
    EngineBase::Renderer().SetMatricesUniforms();
    // draw elements per mesh
    DrawMeshes();

    for (auto &node : nodes)
    {
        node->DrawRecursive();
    }
}

void Node::DrawList()
{
    static const auto &camera = Camera::Active();
    static const auto &renderer = EngineBase::Renderer();

    // draw elements using draw list
    for (auto node : drawList)
    {
        // set this node as rendering active
        node->SetAsActive();

        if (!camera->InFrustum(node->boundaries))
        {
            continue;
        }

        // recalculate model-dependent transform matrices
        node->ComputeMatrices();
        // set matrices uniform with updated matrices
        renderer.SetMatricesUniforms();
        // draw node meshes
        node->DrawMeshes();
    }
}

void Node::ComputeMatrices()
{
    static const auto &camera = Camera::Active();
    modelViewMatrix = camera->ViewMatrix() * modelMatrix;
    normalMatrix = modelViewMatrix;
    modelViewProjectionMatrix = camera->ProjectionMatrix() * modelViewMatrix;
}

void Node::Transform(const glm::vec3 &position, const glm::vec3 &scaling,
                     const glm::quat &rotation)
{
    this->position = position;
    this->scaling = scaling;
    this->rotation = rotation;
    ComputeModelMatrix();
}

void Node::Position(const glm::vec3 &position)
{
    if (position != this->position)
    {
        this->position = position;
        ComputeModelMatrix();
    }
}

void Node::Scaling(const glm::vec3 &scaling)
{
    if (scaling != this->scaling)
    {
        this->scaling = scaling;
        ComputeModelMatrix();
        UpdateBoundaries();
    }
}

void Node::UpdateBoundaries()
{
    boundaries.Transform(modelMatrix);

    for (auto &mesh : meshes)
    {
        mesh->boundaries.Transform(modelMatrix);
    }
}

void Node::Rotation(const glm::quat &rotation)
{
    if (rotation != this->rotation)
    {
        this->rotation = rotation;
        ComputeModelMatrix();
        boundaries.Transform(modelMatrix);
    }
}

void Node::BuildDrawList()
{
    drawList.clear();
    BuildDrawList(drawList);
}

const glm::mat4x4 &Node::NormalMatrix() const
{
    return normalMatrix;
}

const glm::mat4x4 &Node::ModelViewMatrix() const
{
    return modelViewMatrix;
}

const glm::mat4x4 &Node::ModelViewProjectionMatrix() const
{
    return modelViewProjectionMatrix;
}
