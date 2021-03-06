//
//  CallableScoper.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 07/05/16.
//  Copyright © 2016 Theo Weidmann. All rights reserved.
//

#include "VariableNotFoundError.hpp"
#include "Functions/Function.hpp"
#include "Scoping/SemanticScoper.hpp"
#include "Types/TypeDefinition.hpp"
#include "Compiler.hpp"
#include "Analysis/PathAnalyser.hpp"
#include <map>

namespace EmojicodeCompiler {

Scope& SemanticScoper::pushArgumentsScope(PathAnalyser *analyser, const std::vector<Parameter> &arguments,
                                          const SourcePosition &p) {
    pushScope();
    for (auto &variable : arguments) {
        auto &var = currentScope().declareVariable(variable.name, variable.type->type(), true, p);
        analyser->record(PathAnalyserIncident(false, var.id()));
    }
    return currentScope();
}

void SemanticScoper::popScope(PathAnalyser *pathAnalyser, Compiler *compiler) {
    currentScope().checkScope(pathAnalyser, compiler);

    updateMaxVariableIdForPopping();
    scopes_.pop_front();
}

SemanticScopeStats SemanticScoper::createStats() const {
    assert(!scopes_.empty());
    auto count = scopes_.front().map().size();
    auto maxVariableId = scopes_.front().maxVariableId();
    return { scopes_.size() > 1 ? (++scopes_.begin())->maxVariableId() : 0, count, maxVariableId };
}

SemanticScoper SemanticScoper::scoperForFunction(Function *function)  {
    if (hasInstanceScope(function->functionType())) {
        return SemanticScoper(&function->owner()->instanceScope());
    }
    return SemanticScoper();
}

void SemanticScoper::pushScope() {
    scopes_.emplace_front(Scope(scopes_.empty() ? maxVariableId_ : scopes_.front().maxVariableId()));
}

ResolvedVariable SemanticScoper::getVariable(const std::u32string &name, const SourcePosition &errorPosition) {
    for (Scope &scope : scopes_) {
        if (scope.hasLocalVariable(name)) {
            return ResolvedVariable(scope.getLocalVariable(name), false);
        }
    }
    if (instanceScope_ != nullptr && instanceScope_->hasLocalVariable(name)) {
        return ResolvedVariable(instanceScope_->getLocalVariable(name), true);
    }
    throw VariableNotFoundError(errorPosition, name);
}

void SemanticScoper::checkForShadowing(const std::u32string &name, const SourcePosition &p, Compiler *compiler) const {
    for (const Scope &scope : scopes_) {
        if (scope.hasLocalVariable(name)) {
            compiler->warn(p, "Declaration of ", utf8(name), " shadows previous local variable.");
        }
    }
    if (instanceScope_ != nullptr && instanceScope_->hasLocalVariable(name)) {
        compiler->warn(p, "Declaration of ", utf8(name), " shadows instance variable.");
    }
}

}  // namespace EmojicodeCompiler
