"""Execute the notebook and retain outputs, including failures."""
from pathlib import Path
import nbformat
from nbclient import NotebookClient

path = Path(__file__).resolve().with_name("parts5_7.ipynb")
notebook = nbformat.read(path, as_version=4)
client = NotebookClient(notebook, timeout=180, kernel_name="python3",
                        resources={"metadata": {"path": str(path.parent)}})
try:
    client.execute()
finally:
    nbformat.write(notebook, path)
print(f"Executed {path}")
