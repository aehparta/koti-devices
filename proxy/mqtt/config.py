import yaml

def load(filename = 'config.yaml'):
    with open(filename, 'r') as f:
        cfg = yaml.safe_load(f)
        return cfg
