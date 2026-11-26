import os
import sys
import pytest
from pathlib import Path

# Add backend directory to sys.path
BASE_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(BASE_DIR))

from app import app, ensure_engine_compiled

@pytest.fixture(scope="session", autouse=True)
def setup_c_engine():
    """Ensure the C tree engine executable is compiled before running any tests."""
    ok, msg = ensure_engine_compiled()
    assert ok, f"Setup failure: C engine compilation failed: {msg}"

@pytest.fixture
def client():
    """Flask test client fixture."""
    app.config["TESTING"] = True
    with app.test_client() as client:
        yield client
