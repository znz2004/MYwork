import inspect
from typing import Dict, List


class CommandShell:
    """交互式命令 Shell"""
    
    def __init__(self, container):
        self.container = container
        self.commands = {}
        self._scan_commands()
    
    def _scan_commands(self):
        for cls, reg_info in self.container._registry.items():
            layer = reg_info.get('layer')
            
            if layer != "presentation":
                continue
            
            instance = self.container.get(cls)
            
            for method_name in dir(instance):
                if method_name.startswith('_'):
                    continue
                
                method = getattr(instance, method_name)
                if callable(method) and hasattr(method, '__command__'):
                    cmd_name = getattr(method, '__command_name__', method_name)
                    description = getattr(method, '__command_description__', '')
                    
                    sig = inspect.signature(method)
                    params = []
                    for param_name, param in sig.parameters.items():
                        if param_name == 'self':
                            continue
                        param_type = param.annotation
                        if param_type == inspect.Parameter.empty:
                            param_type = str
                        params.append({
                            'name': param_name,
                            'type': param_type,
                        })
                    
                    self.commands[cmd_name] = {
                        'handler': method,
                        'description': description,
                        'params': params
                    }
    
    def run(self):
        print("\n" + "=" * 60)
        print("POS 系统已启动")
        print("输入 'help' 查看可用命令，'quit' 退出")
        print("=" * 60)
        
        while True:
            try:
                user_input = input("\nPOS> ").strip()
                if not user_input:
                    continue
                
                if user_input.lower() == 'quit':
                    print("正在关闭...")
                    self.container.close()
                    break
                
                if user_input.lower() == 'help':
                    self._show_help()
                    continue
                
                parts = user_input.split()
                cmd_name = parts[0].lower()
                args = parts[1:]
                
                if cmd_name not in self.commands:
                    print(f"未知命令: {cmd_name}，输入 'help' 查看可用命令")
                    continue
                
                cmd_info = self.commands[cmd_name]
                self._execute_command(cmd_info, args)
                
            except KeyboardInterrupt:
                print("\n\n正在关闭...")
                self.container.close()
                break
            except Exception as e:
                print(f"执行出错: {e}")
    
    def _show_help(self):
        print("\n可用命令:")
        print("-" * 50)
        for cmd_name, cmd_info in sorted(self.commands.items()):
            params_str = ' '.join([f"<{p['name']}>" for p in cmd_info['params']])
            print(f"  {cmd_name} {params_str:<30} {cmd_info['description']}")
        print("-" * 50)
        print("  help                             显示帮助")
        print("  quit                             退出")
    
    def _execute_command(self, cmd_info: Dict, args: List[str]):
        params = cmd_info['params']
        
        if len(args) != len(params):
            param_names = ' '.join([f"<{p['name']}>" for p in params])
            print(f"参数错误！用法: {cmd_info['handler'].__command_name__} {param_names}")
            return
        
        converted_args = []
        for i, arg in enumerate(args):
            param_type = params[i]['type']
            try:
                if param_type == int:
                    converted_args.append(int(arg))
                elif param_type == float:
                    converted_args.append(float(arg))
                else:
                    converted_args.append(arg)
            except ValueError:
                print(f"参数 {params[i]['name']} 应为 {param_type.__name__} 类型")
                return
        
        try:
            result = cmd_info['handler'](*converted_args)
            if result:
                print(result)
        except Exception as e:
            print(f"执行失败: {e}")