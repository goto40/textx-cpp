#include "textx/model.h"

namespace textx::model {

    Model::Model(const textx::arpeggio::Match& parsetree) {
        root = create_model(parsetree);
    } 

    textx::object::Value Model::create_model(const textx::arpeggio::Match &m) {
        
    }

}