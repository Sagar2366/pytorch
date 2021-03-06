#include <torch/csrc/jit/passes/lower_grad_of.h>

namespace torch {
namespace jit {

void LowerGradOf(Graph& g) {
  for (auto it = g.nodes().begin(); it != g.nodes().end(); ++it) {
    if (it->kind() == prim::GradOf) {
      // if any_defined(inputs):
      //  outputs = <original_computation>
      // else:
      //  outputs = undefineds
      WithInsertPoint guard(*it);
      auto cond = g.insertNode(g.create(prim::AnyDefined, it->inputs()))
                      ->output()
                      ->setType(IntType::get());
      auto if_stat =
          g.insertNode(g.create(prim::If, {cond}, it->outputs().size()));
      if_stat->addBlock()->cloneFrom(
          it->blocks().at(0), [](Value* v) { return v; });
      auto else_block = if_stat->addBlock();
      auto undef = g.createUndefined()
                       ->insertBefore(else_block->return_node())
                       ->output();
      for (size_t i = 0; i < it->outputs().size(); ++i) {
        else_block->registerOutput(undef);
        if_stat->outputs().at(i)->copyMetadata(it->outputs().at(i));
      }
      it->replaceAllUsesWith(if_stat);
      it.destroyCurrent();
    }
  }
}

} // namespace jit
} // namespace torch
