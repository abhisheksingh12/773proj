#!/usr/bin/env python2.7
import sys,pickle,traceback

global DEBUG
global DEBUGcounter
DEBUG = False


class SUBJCLUESproc:
    
    def __init__(self,pathToPickle,pathToSubjclues=None):
        
        if DEBUG:
            self.DEBUGcounter = 0
        
        if pathToSubjclues != None:
            
            stemmedDict = {}
            tokenDict   = {}
            file_h = open(pathToSubjclues)
            for line in file_h:
                
                parts = line.split(' ')
                
                tmp_dic = {}
                for part in parts:
                    """
                    Error:
                    type=strongsubj len=1 word1=pervasive pos1=adj stemmed1=n m priorpolarity=negative
                    has a weird dangling 'm;
                    """
                    if part == 'm':
                        continue
                    
                    (title,val) = part.split('=')
                    
                    tmp_dic[title] = val.rstrip()
                
                if tmp_dic['stemmed1']=='y':
                    final_dict = stemmedDict
                    """
                    there is are ERRORS in the file where stemmed1 has a value of 1 instead of (y|n)
                    
                    type=strongsubj len=1 word1=lugubrious pos1=adj stemmed1=1 priorpolarity=negative
                    

                    """
                elif tmp_dic['stemmed1']=='n' or tmp_dic['stemmed1']=='1':
                    final_dict = tokenDict

                
                else:
                    print line
                    raise Exception("Problem Parsing file %s"%pathToSubjclues)
                
                key = tmp_dic['word1']
                pos = tmp_dic['pos1']
                if key not in final_dict:
                    final_dict[key] = {}
                
                if DEBUG:
                    """
                    or there are repeated lines like:
                    
                    type=strongsubj len=1 word1=autocratic pos1=adj stemmed1=n priorpolarity=negative
                    type=strongsubj len=1 word1=autocratic pos1=adj stemmed1=n priorpolarity=negative
                    """
                    if pos in final_dict[key]:
                        self.DEBUGcounter += 1
                        print "%d Repeated line in file: \n%s\n"%(self.DEBUGcounter,line)
                        
                
                final_dict[key][pos]={'type':tmp_dic['type'],'priorpolarity':tmp_dic['priorpolarity']}                
                del tmp_dic
                file_h.close()
            self.SUBJCLUES = {'stemmed':stemmedDict,'token':tokenDict}
            pickle.dump(self.SUBJCLUES, open(pathToPickle,"wb"))
        else:
            self.SUBJCLUES = pickle.load(open(pathToPickle))

    def inquire(self,word,word_pos):
        """
        I am looking for a match in the token dict
        first if I cannot find a match then I look
        in the stemmed dict.
        
        NOTE: 
        -Not dealing with multiple matches
        matches in both the token and the stemmed dicts
        
        -If the POS tag does not match, I am not
        returning anything even though the word 
        might be in the resource
        
        RETURNING a list in the following order
        [type,pos,priorpolarity]
        """
        tmp_dic = None
        if word in self.SUBJCLUES['token']:
            tmp_dic = self.SUBJCLUES['token'][word]
        else:
            for key in self.SUBJCLUES['stemmed']:
                if key in word:
                    tmp_dic = self.SUBJCLUES['stemmed'][key]
                    break
        if tmp_dic == None:
            return []
        else:
            
            if word_pos in tmp_dic: 
                return [tmp_dic[word_pos]['type'],tmp_dic[word_pos]['priorpolarity']]
            else:
                return []
            
    def tagWITHOUTPOS(self,word):
        """
        I am looking for a match in the token dict
        first if I cannot find a match then I look
        in the stemmed dict.
        
        NOTE: 
        -Not dealing with multiple matches
        matches in both the token and the stemmed dicts
        
        -If the POS tag does not match, I am not
        returning anything even though the word 
        might be in the resource
        
        RETURNING a list in the following order
        [type,pos,priorpolarity]
        """
        tmp_dic = None
        if word in self.SUBJCLUES['token']:
            tmp_dic = self.SUBJCLUES['token'][word]
        else:
            for key in self.SUBJCLUES['stemmed']:
                if key in word:
                    tmp_dic = self.SUBJCLUES['stemmed'][key]
                    break
        if tmp_dic == None:
            return []
        else:
            if len(tmp_dic)==1:
                key = tmp_dic.keys()[0]
                return [word+"_"+tmp_dic[key]['type'].upper(),word+"_"+tmp_dic[key]['priorpolarity'].upper()]
            else:
                return []
            
        
    def inquireWITHOUTPOS(self,word):
        """
        I am looking for a match in the token dict
        first if I cannot find a match then I look
        in the stemmed dict.
        
        NOTE: 
        -Not dealing with multiple matches
        matches in both the token and the stemmed dicts
        
        -If the POS tag does not match, I am not
        returning anything even though the word 
        might be in the resource
        
        RETURNING a list in the following order
        [type,pos,priorpolarity]
        """
        tmp_dic = None
        if word in self.SUBJCLUES['token']:
            tmp_dic = self.SUBJCLUES['token'][word]
        else:
            for key in self.SUBJCLUES['stemmed']:
                if key in word:
                    tmp_dic = self.SUBJCLUES['stemmed'][key]
                    break
        if tmp_dic == None:
            return []
        else:
            if len(tmp_dic)==1:
                key = tmp_dic.keys()[0]
                return [tmp_dic[key]['type'].upper(),tmp_dic[key]['priorpolarity'].upper()]
            else:
                return []
            
      
            
if __name__ == "__main__":
    
    try:
        pickle_f = sys.argv[1]
        
        if len(sys.argv) == 5:
            
            resource = sys.argv[2]
            word = sys.argv[3]
            pos = sys.argv[4]
        else: #len(sys.argv) == 3:
            
            resource = None
            word     = sys.argv[2]
            pos     = sys.argv[3]
        
        subCl = SUBJCLUESproc(pickle_f,resource)
        
        info = subCl.inquire(word,pos)
        
        string = "%s\t"%word
        for category in info:
            string += category+"\t"
        
        string = string[:-2]+"\n"
        sys.stdout.write(string)
        
        """
        ERASE ME
        |
        |
        V
        """
        print str(subCl.inquireWITHOUTPOS(word))
        
    except:
        print ('Usage: pickle_path path_to_SUBJCLUES_resource word \nor\n'+\
               'Usage: pickle_path word \n')
        print '-'*60
        traceback.print_exc(file=sys.stdout)
        print '-'*60
        
