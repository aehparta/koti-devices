import yaml

__cfg = None


def load(filename='config.yaml'):
    global __cfg
    with open(filename, 'r') as f:
        __cfg = yaml.safe_load(f)


def get_recursively(node, parts, default):
    if parts[0] not in node:
        return default
    if len(parts) == 1:
        return node[parts[0]]
    return get_recursively(node[parts[0]], parts[1:], default)


def get(name: str, default=None):
    global __cfg
    return get_recursively(__cfg, name.split('.'), default)
