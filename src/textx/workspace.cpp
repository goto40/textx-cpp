#include "workspace.h"
namespace textx {
    std::shared_ptr<Workspace> Workspace::create() { 
        return textx::WorkspaceImpl<std::shared_ptr>::create();
    }
}