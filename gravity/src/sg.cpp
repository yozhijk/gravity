#include "sg.h"

namespace  Gravity
{
    DefaultSceneGraph *CreateDefaultSceneGraph(DefaultSceneGraph::ParameterFactory *factory)
    {
        return new DefaultSceneGraph(factory);
    }
}


