# Contributing to BEEP Base Firmware

Thank you for your interest in contributing to the BEEP Base firmware project. This document provides guidelines and information for contributors.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md) to maintain a respectful and inclusive community.

## Development Process

### 1. Setting Up Development Environment

```bash
# Clone repository with west
west init -m https://github.com/your-repo/beep-base-zephyr
west update

# Install development dependencies
pip3 install -r scripts/requirements-dev.txt
```

### 2. Branch Naming Convention

- `feature/` - New features
- `bugfix/` - Bug fixes
- `docs/` - Documentation updates
- `test/` - Test additions or modifications
- `refactor/` - Code refactoring

Example: `feature/cellular-power-optimization`

### 3. Commit Message Format

```
type(scope): Brief description

Detailed description of changes and reasoning.

Fixes #123
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code restructuring
- `test`: Test updates
- `chore`: Maintenance

Example:
```
feat(cellular): Add PSM configuration support

Implement Power Saving Mode configuration for the nRF9161 modem
with automatic timing adjustment based on transmission intervals.

Fixes #45
```

## Code Style Guidelines

### C Code Style

```c
// Function naming - snake_case
int cellular_app_init(void);

// Constants - UPPER_CASE
#define MAX_RETRY_COUNT 3

// Variables - snake_case
static uint32_t retry_counter;

// Structs - snake_case with _t suffix
typedef struct {
    uint8_t state;
    uint32_t timestamp;
} cellular_state_t;

// Enums - UPPER_CASE
typedef enum {
    POWER_STATE_ACTIVE,
    POWER_STATE_SLEEP
} power_state_t;
```

### Documentation Style

```c
/**
 * @brief Initialize cellular communication
 *
 * @param config Pointer to configuration structure
 * @return 0 on success, negative errno on failure
 *
 * This function initializes the cellular modem with the specified
 * configuration. It sets up power management features and establishes
 * the initial connection.
 */
int cellular_app_init(const cellular_config_t *config);
```

## Testing Requirements

### 1. Unit Tests

```c
// test/unit/test_cellular.c
void test_cellular_init(void)
{
    cellular_config_t config = {
        .psm_enabled = true,
        .edrx_enabled = true
    };
    
    TEST_ASSERT_EQUAL(0, cellular_app_init(&config));
    TEST_ASSERT_EQUAL(CELLULAR_STATE_INIT, cellular_app_get_state());
}
```

### 2. Integration Tests

```bash
# Run integration tests
./scripts/test.sh --integration

# Run specific test suite
./scripts/test.sh --suite cellular
```

### 3. Power Consumption Tests

```bash
# Run power profile test
./scripts/test_power.sh --duration 3600

# Validate against baseline
./scripts/validate_power.sh --profile latest.csv
```

## Pull Request Process

1. **Create Issue**
   - Describe the problem or enhancement
   - Get feedback from maintainers
   - Link related issues

2. **Development**
   - Create feature branch
   - Make changes
   - Add tests
   - Update documentation

3. **Testing**
   ```bash
   # Run all tests
   ./scripts/test_all.sh
   
   # Verify coding style
   ./scripts/check_style.sh
   
   # Build documentation
   ./scripts/build_docs.sh
   ```

4. **Submit PR**
   - Fill out PR template
   - Link related issues
   - Add test results
   - Request review

## Review Process

### 1. Code Review Checklist

- [ ] Follows coding style
- [ ] Includes tests
- [ ] Documentation updated
- [ ] No regression issues
- [ ] Power efficiency maintained
- [ ] Error handling complete

### 2. Testing Requirements

- Unit tests pass
- Integration tests pass
- Power consumption within limits
- No memory leaks
- No timing issues

### 3. Documentation Requirements

- API documentation complete
- Changes documented
- Examples provided
- Power implications noted

## Release Process

### 1. Version Numbering

We use Semantic Versioning:
- MAJOR.MINOR.PATCH
- Example: 1.2.3

### 2. Release Checklist

```bash
# Update version
./scripts/update_version.sh 1.2.3

# Run full test suite
./scripts/test_all.sh

# Build release
./scripts/build_release.sh

# Generate documentation
./scripts/build_docs.sh
```
