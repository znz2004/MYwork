class CircularDependencyError(Exception):
    """循环依赖异常"""
    def __init__(self, chain: list):
        self.chain = chain
        chain_str = " -> ".join([c.__name__ for c in chain])
        super().__init__(f"CircularDependencyError: {chain_str}")


class LayerViolationError(Exception):
    """层间依赖违规异常"""
    def __init__(self, source_layer: str, source_name: str, 
                 target_layer: str, target_name: str, allowed: list):
        self.source_layer = source_layer
        self.source_name = source_name
        self.target_layer = target_layer
        self.target_name = target_name
        self.allowed = allowed
        
        msg = (f"LayerViolationError: {source_name} ({source_layer}) -> "
               f"{target_name} ({target_layer})\n"
               f"  {source_layer} can only depend on: {allowed}")
        super().__init__(msg)