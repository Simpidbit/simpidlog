from pathlib import Path

from setuptools import find_packages, setup


ROOT = Path(__file__).parent


setup(
    name="simpidlog",
    version="0.0.1",
    author="Simpidbit Isaiah",
    author_email="simpidbit@gmail.com",
    description="A simple multithreading logger.",
    long_description=(ROOT / "README.md").read_text(encoding="utf-8"),
    long_description_content_type="text/markdown",
    url="https://github.com/Simpidbit/simpidlog",
    license="MIT",
    package_dir={"": "src"},
    packages=find_packages(where="src"),
    install_requires=["tqdm"],
    python_requires=">=3.7",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)
