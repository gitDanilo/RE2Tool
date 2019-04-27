#Delete local branch:
git branch -d <branch_name>
git branch -D <branch_name>

#Delete remote branch:
git push origin -d <branch_name>

#Remove obsolete tracking branches
git fetch --all --prune

#Create a new branch
git checkout -b <new_branch> <root_branch>

#Merge branch
git checkout <root_branch>
git merge <branch_name>

#Display git tree
git log --graph --oneline --all