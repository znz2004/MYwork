class CircularDependencyError(Exception):
    """循环依赖异常"""
    def __init__(self, chain: list):
        self.chain = chain
        chain_str = " -> ".join([c.__name__ for c in chain])
        super().__init__(f"CircularDependencyError: {chain_str}")