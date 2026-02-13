import pandas as pd
from sklearn.model_selection import train_test_split

# Load the CSV you downloaded
df = pd.read_csv("Iris.csv")

# Last column is label, keep everything else as features
feature_cols = df.columns[1:-1]  # skip original Id column
label_col = df.columns[-1]

X = df[feature_cols]
y = df[label_col]

# Convert features to integers (fixed-point: multiply by 100)
X_int = (X * 100).round().astype(int)

# Stratified split: 80% train, 20% test
X_train, X_test, y_train, y_test = train_test_split(
    X_int, y, test_size=0.2, stratify=y, random_state=42
)

# Combine features + labels again
train_df = X_train.copy()
train_df['Species'] = y_train
test_df = X_test.copy()
test_df['Species'] = y_test

# Add a sequential Id column
train_df.insert(0, 'Id', range(1, len(train_df) + 1))
test_df.insert(0, 'Id', range(1, len(test_df) + 1))

# Save CSVs
train_df.to_csv("train.csv", index=False)
test_df.to_csv("test.csv", index=False)

print(f"Train set: {len(train_df)} samples")
print(f"Test set: {len(test_df)} samples")
