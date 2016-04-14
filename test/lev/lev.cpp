#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <time.h>
#include <locale>
#include <codecvt>
#include <cwctype>
using namespace std;
extern "C" {
    int __declspec(dllexport) matchscore(const char* s8, const char* t8)
    {
        // string s ="鲁B11111";
        // string t ="京C22222";
        // std::locale loc("");
        std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> cv;
        std::u32string s = cv.from_bytes(s8);
        std::u32string t = cv.from_bytes(t8);
        

        // char array[8][8];
        vector<vector<int>> d;

        int m = s.length();
        int n = t.length();

        d.resize(m+1);
        for(int i=0; i<=m; i++) {
            d[i].resize(n+1, 0);
        }

        for(int i=1; i<=m; i++) {
            d[i][0] = i;
        }
        for(int j=1; j<=n; j++) {
            d[0][j] = j;
        }
        for(int j=1; j<=n; j++) {
            for(int i=1; i<=m; i++) {
                if (std::towupper(s[i-1]) == std::towupper(t[j-1])) {
                    d[i][j]=d[i-1][j-1];
                }
                else {
                    d[i][j]=min(d[i-1][j]+1, min(d[i][j-1]+1,d[i-1][j-1]+1));
                }
            }
        }
        return d[m][n];
    }
};

