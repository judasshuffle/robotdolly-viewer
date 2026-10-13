/**
 * ROBOT DOLLY experimental workspace model.
 *
 * This file deliberately has no dependencies on the SL rendering, LLUI or
 * networking systems.  It describes editor *instances* and a resizable split
 * tree.  Actual LLView hosts and render targets are a later integration step.
 *
 * Copyright (c) 2026 ROBOT DOLLY contributors.
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#ifndef ROBOTDOLLY_WORKSPACE_MODEL_H
#define ROBOTDOLLY_WORKSPACE_MODEL_H

#include <algorithm>
#include <cstdint>
#include <memory>
#include <istream>
#include <ostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace robotdolly
{
using AreaId = std::uint64_t;

struct EditorDescriptor
{
    std::string id;
    std::string display_name;
    bool allow_multiple = true;
};

class EditorRegistry
{
public:
    bool registerEditor(EditorDescriptor descriptor)
    {
        if (descriptor.id.empty() || descriptor.display_name.empty())
            return false;
        const auto key = descriptor.id;
        return mEditors.emplace(key, std::move(descriptor)).second;
    }

    const EditorDescriptor* find(const std::string& id) const
    {
        const auto it = mEditors.find(id);
        return it == mEditors.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, EditorDescriptor> mEditors;
};

enum class SplitAxis { Horizontal, Vertical };

struct Area
{
    AreaId id = 0;
    // Stable editor type identifier, not tied to the area's location.
    std::string editor_type;
    // Optional data key: e.g. local chat, an IM session UUID, or landmark folder.
    std::string context_key;
};

struct LayoutNode
{
    Area area;
    bool is_split = false;
    SplitAxis axis = SplitAxis::Horizontal;
    float fraction = 0.5f;
    std::unique_ptr<LayoutNode> first;
    std::unique_ptr<LayoutNode> second;
};

class Workspace
{
public:
    explicit Workspace(const EditorRegistry& registry) : mRegistry(registry)
    {
        mRoot = std::make_unique<LayoutNode>();
        mRoot->area = {mNextAreaId++, "sl.world", ""};
    }

    const LayoutNode& root() const { return *mRoot; }
    void save(std::ostream& out) const {
        out << "RD_WORKSPACE 1\n";
        saveNode(out, *mRoot);
    }
    bool load(std::istream& in) {
        std::string magic; int version=0;
        if (!(in >> magic >> version) || magic != "RD_WORKSPACE" || version != 1) return false;
        AreaId high=0; std::unordered_set<AreaId> ids;
        auto next=loadNode(in,0,high,ids);
        if (!next || ids.empty()) return false;
        mRoot=std::move(next); mNextAreaId=high+1; return true;
    }

    // Returns the new area's identifier, or zero if the split is invalid.
    AreaId split(AreaId existing_id, SplitAxis axis, const std::string& new_editor)
    {
        if (!mRegistry.find(new_editor))
            return 0;
        LayoutNode* node = findNode(mRoot.get(), existing_id);
        if (!node || node->is_split)
            return 0;

        Area original = std::move(node->area);
        const AreaId created_id = mNextAreaId++;
        node->is_split = true;
        node->axis = axis;
        node->fraction = 0.5f;
        node->area = {};
        node->first = std::make_unique<LayoutNode>();
        node->first->area = std::move(original);
        node->second = std::make_unique<LayoutNode>();
        node->second->area = {created_id, new_editor, ""};
        return created_id;
    }

    bool setEditor(AreaId id, const std::string& editor_type,
                   const std::string& context_key = "")
    {
        if (!mRegistry.find(editor_type))
            return false;
        LayoutNode* node = findNode(mRoot.get(), id);
        if (!node || node->is_split)
            return false;
        node->area.editor_type = editor_type;
        node->area.context_key = context_key;
        return true;
    }

    bool resize(AreaId descendant_id, float fraction)
    {
        // Resizes the immediate parent split of the specified leaf.
        // Keep both areas usable; pixel minimums belong to the UI adapter.
        LayoutNode* parent = findParent(mRoot.get(), descendant_id);
        if (!parent)
            return false;
        parent->fraction = std::clamp(fraction, 0.1f, 0.9f);
        return true;
    }

    // Resize the split separating two different descendant areas.
    bool resizeBetween(AreaId first_id, AreaId second_id, float fraction)
    {
        LayoutNode* split_node = findSeparatingSplit(mRoot.get(), first_id, second_id);
        if (!split_node) return false;
        split_node->fraction = std::clamp(fraction, 0.1f, 0.9f);
        return true;
    }

    bool closeArea(AreaId id)
    {
        // Collapse the parent split, retaining its sibling.
        return removeLeaf(mRoot, id);
    }

private:
    static void saveNode(std::ostream& out,const LayoutNode& n) {
        if (!n.is_split) {
            out << "A " << n.area.id << ' ' << std::quoted(n.area.editor_type)
                << ' ' << std::quoted(n.area.context_key) << '\n';
        } else {
            out << "S " << (n.axis==SplitAxis::Horizontal?'H':'V') << ' ' << n.fraction << '\n';
            saveNode(out,*n.first); saveNode(out,*n.second);
        }
    }
    std::unique_ptr<LayoutNode> loadNode(std::istream& in,int depth,
                          AreaId& high,std::unordered_set<AreaId>& ids) const {
        if(depth>64 || ids.size()>1024) return nullptr;
        char kind=0; if(!(in>>kind))return nullptr;
        auto n=std::make_unique<LayoutNode>();
        if(kind=='A') {
            if(!(in>>n->area.id>>std::quoted(n->area.editor_type)
                   >>std::quoted(n->area.context_key))) return nullptr;
            if(!n->area.id || !ids.insert(n->area.id).second) return nullptr;
            if(!mRegistry.find(n->area.editor_type))n->area.editor_type="sl.world";
            high=std::max(high,n->area.id);return n;
        }
        if(kind!='S')return nullptr;
        char axis=0;if(!(in>>axis>>n->fraction))return nullptr;
        if((axis!='H'&&axis!='V') || !std::isfinite(n->fraction) ||
           n->fraction<0.1f || n->fraction>0.9f)return nullptr;
        n->is_split=true;
        n->axis=axis=='H'?SplitAxis::Horizontal:SplitAxis::Vertical;
        n->first=loadNode(in,depth+1,high,ids);
        if(!n->first)return nullptr;
        n->second=loadNode(in,depth+1,high,ids);
        if(!n->second)return nullptr;
        return n;
    }
    static LayoutNode* findNode(LayoutNode* node, AreaId id)
    {
        if (!node)
            return nullptr;
        if (!node->is_split)
            return node->area.id == id ? node : nullptr;
        if (auto* found = findNode(node->first.get(), id))
            return found;
        return findNode(node->second.get(), id);
    }

    static bool containsLeaf(const LayoutNode* node, AreaId id)
    {
        if (!node) return false;
        if (!node->is_split) return node->area.id == id;
        return containsLeaf(node->first.get(), id) ||
               containsLeaf(node->second.get(), id);
    }

    static LayoutNode* findSeparatingSplit(LayoutNode* node, AreaId a, AreaId b)
    {
        if (!node || !node->is_split) return nullptr;
        if (containsLeaf(node->first.get(), a) && containsLeaf(node->second.get(), b))
            return node;
        if (auto* found = findSeparatingSplit(node->first.get(), a, b)) return found;
        return findSeparatingSplit(node->second.get(), a, b);
    }

    static LayoutNode* findParent(LayoutNode* node, AreaId id)
    {
        if (!node || !node->is_split) return nullptr;
        if ((node->first && !node->first->is_split && node->first->area.id == id) ||
            (node->second && !node->second->is_split && node->second->area.id == id))
            return node;
        if (auto* found = findParent(node->first.get(), id)) return found;
        return findParent(node->second.get(), id);
    }

    static bool removeLeaf(std::unique_ptr<LayoutNode>& node, AreaId id)
    {
        if (!node || !node->is_split) return false;
        if (node->first && !node->first->is_split && node->first->area.id == id)
        {
            node = std::move(node->second);
            return true;
        }
        if (node->second && !node->second->is_split && node->second->area.id == id)
        {
            node = std::move(node->first);
            return true;
        }
        return removeLeaf(node->first, id) || removeLeaf(node->second, id);
    }

    const EditorRegistry& mRegistry;
    std::unique_ptr<LayoutNode> mRoot;
    AreaId mNextAreaId = 1;
};

inline EditorRegistry makeDefaultRegistry()
{
    EditorRegistry registry;
    registry.registerEditor({"sl.world", "Second Life World", true});
    registry.registerEditor({"sl.chat", "Conversation", true});
    registry.registerEditor({"sl.inventory", "Inventory", true});
    registry.registerEditor({"sl.landmarks", "Landmarks", true});
    return registry;
}

} // namespace robotdolly
#endif // ROBOTDOLLY_WORKSPACE_MODEL_H
